#include "flash/update_module.hpp"

#include "flash/gzip_reader.hpp"
#include "flash/logger.hpp"
#include "flash/partition_writer.hpp"
#include "flash/archive_installer.hpp"

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace flash {

namespace {

static bool EndsWithGz(const std::string& s) {
    return s.size() >= 3 && s.compare(s.size() - 3, 3, ".gz") == 0;
}

// Counts bytes read from the *bundle entry stream* (compressed bytes if the entry is .gz).
class CountingReader final : public IReader {
public:
    CountingReader(std::unique_ptr<IReader> inner, std::uint64_t* counter)
        : inner_(std::move(inner)), counter_(counter) {}

    ssize_t Read(std::span<std::uint8_t> out) override {
        const ssize_t n = inner_->Read(out);
        if (n > 0 && counter_) *counter_ += static_cast<std::uint64_t>(n);
        return n;
    }

    std::optional<std::uint64_t> TotalSize() const override {
        return inner_ ? inner_->TotalSize() : std::nullopt;
    }

private:
    std::unique_ptr<IReader> inner_;
    std::uint64_t* counter_ = nullptr;
};

static void EmitProgress(const UpdateModule::Options& opt,
                         const char* tag,
                         std::uint64_t in_done,
                         std::uint64_t out_written,
                         bool final) {
    if (!opt.progress) return;

    // Component %
    if (opt.component_total_bytes > 0) {
        const int pct = static_cast<int>((in_done * 100ULL) / opt.component_total_bytes);

        if (opt.overall_total_bytes > 0) {
            const std::uint64_t overall_done = opt.overall_done_base_bytes + in_done;
            const int opct = static_cast<int>((overall_done * 100ULL) / opt.overall_total_bytes);

            LogInfo("[%s] %s OTA:%d%% COMP:%d%% (in %llu/%llu, out %llu)",
                    tag,
                    final ? "done" : "progress",
                    opct,
                    pct,
                    (unsigned long long)in_done,
                    (unsigned long long)opt.component_total_bytes,
                    (unsigned long long)out_written);
        } else {
            LogInfo("[%s] %s %d%% (in %llu/%llu, out %llu)",
                    tag,
                    final ? "done" : "progress",
                    pct,
                    (unsigned long long)in_done,
                    (unsigned long long)opt.component_total_bytes,
                    (unsigned long long)out_written);
        }
    } else {
        // Unknown total => show bytes
        LogInfo("[%s] %s (in %llu bytes, out %llu bytes)",
                tag,
                final ? "done" : "progress",
                (unsigned long long)in_done,
                (unsigned long long)out_written);
    }
}

} // namespace

Result UpdateModule::Execute(const Component& comp, std::unique_ptr<IReader> source, const Options& opt) {
    if (!source) return Result::Fail(-1, "Null source reader");

    const char* tag = comp.name.c_str();

    LogInfo("UpdateModule: name=%s type=%s file=%s",
            comp.name.c_str(), comp.type.c_str(), comp.filename.c_str());

    // Count bundle-entry bytes read (compressed if .gz).
    std::uint64_t in_read = 0;
    std::unique_ptr<IReader> effective_reader =
        std::make_unique<CountingReader>(std::move(source), &in_read);

    // Wrap gzip if needed (gzip reads from CountingReader -> in_read tracks compressed bytes)
    if (EndsWithGz(comp.filename)) {
        try {
            LogDebug("Wrapping GzipReader for %s", comp.filename.c_str());
            effective_reader = std::make_unique<GzipReader>(std::move(effective_reader));
        } catch (const std::exception& e) {
            return Result::Fail(-1, std::string("Gzip init failed: ") + e.what());
        }
    }

    if (comp.type == "raw") {
        return InstallRaw(comp, *effective_reader, opt, tag, &in_read);
    } else if (comp.type == "archive") {
        return InstallArchive(comp, *effective_reader, opt, tag, &in_read);
    } else if (comp.type == "file") {
        return InstallAtomicFile(comp, *effective_reader, opt, tag, &in_read);
    }

    return Result::Fail(-1, "Unsupported component type: " + comp.type);
}

Result UpdateModule::InstallRaw(const Component& comp, IReader& reader, const Options& opt, const char* tag, const std::uint64_t* in_read) {
    if (comp.install_to.empty()) {
        return Result::Fail(-1, "install_to empty for raw component: " + comp.name);
    }

    PartitionWriter writer;
    auto res = PartitionWriter::Open(comp.install_to, writer);
    if (!res.is_ok()) return res;

    return InternalPipe(reader, writer, opt, tag, in_read);
}

Result UpdateModule::InstallArchive(const Component& comp, IReader& reader, const Options& opt, const char* tag, const std::uint64_t* /*in_read*/) {
    // ArchiveInstaller currently logs its own byte-progress; overall % can still be emitted
    // by UpdateModule if you add callbacks there later. For now, keep it simple.
    ArchiveInstaller::Options aopt;
    aopt.progress = opt.progress;
    aopt.progress_interval_bytes = opt.progress_interval_bytes;

    ArchiveInstaller installer(aopt);
    auto r = installer.InstallTarStreamToTarget(reader, comp.install_to, comp.name);
    return r;
}

Result UpdateModule::InstallAtomicFile(const Component& comp, IReader& reader, const Options& opt, const char* tag, const std::uint64_t* in_read) {
    if (comp.path.empty()) {
        return Result::Fail(-1, "File path is empty for component: " + comp.name);
    }

    namespace fs = std::filesystem;
    fs::path dst(comp.path);
    fs::path parent = dst.parent_path();

    if (!parent.empty() && !fs::exists(parent)) {
        if (!comp.create_destination) {
            return Result::Fail(ENOENT,
                "Destination directory does not exist: " + parent.string() +
                " (set 'create-destination': true in manifest)");
        }

        std::error_code ec;
        fs::create_directories(parent, ec);
        if (ec) {
            return Result::Fail(-1, "create_directories failed: " + parent.string() + ": " + ec.message());
        }
        LogInfo("Created destination directory: %s", parent.string().c_str());
    }

    std::string tmp_path = comp.path + ".tmp";

    PartitionWriter writer;
    auto res = PartitionWriter::Open(tmp_path, writer);
    if (!res.is_ok()) return res;

    res = InternalPipe(reader, writer, opt, tag, in_read);
    if (!res.is_ok()) {
        ::unlink(tmp_path.c_str());
        return res;
    }

    if (::rename(tmp_path.c_str(), comp.path.c_str()) != 0) {
        const int err = errno;
        ::unlink(tmp_path.c_str());
        return Result::Fail(err, "Atomic rename failed: " + std::string(std::strerror(err)));
    }

    if (!comp.permissions.empty()) {
        mode_t mode = static_cast<mode_t>(std::stoul(comp.permissions, nullptr, 8));
        if (::chmod(comp.path.c_str(), mode) != 0) {
            const int err = errno;
            return Result::Fail(err, "chmod failed: " + std::string(std::strerror(err)));
        }
    }

    return Result::Ok();
}

Result UpdateModule::InternalPipe(IReader& r, IWriter& w, const Options& opt, const char* tag, const std::uint64_t* in_read) {
    std::vector<std::uint8_t> buffer(1024 * 1024); // 1MB

    std::uint64_t written = 0;
    std::uint64_t next_progress = opt.progress_interval_bytes;
    std::uint64_t next_fsync = opt.fsync_interval_bytes;

    // initial progress line
    EmitProgress(opt, tag, in_read ? *in_read : 0, written, /*final=*/false);

    while (true) {
        const ssize_t n = r.Read(std::span<std::uint8_t>(buffer.data(), buffer.size()));
        if (n == 0) break;
        if (n < 0) return Result::Fail(errno, "Read failed during pipe");

        auto res = w.WriteAll({buffer.data(), static_cast<size_t>(n)});
        if (!res.is_ok()) return res;

        written += static_cast<std::uint64_t>(n);

        const std::uint64_t in_done = in_read ? *in_read : written;

        if (opt.progress && opt.progress_interval_bytes > 0 && in_done >= next_progress) {
            EmitProgress(opt, tag, in_done, written, /*final=*/false);
            next_progress = in_done + opt.progress_interval_bytes;
        }

        if (opt.fsync_interval_bytes > 0 && written >= next_fsync) {
            auto fr = w.FsyncNow();
            if (!fr.is_ok()) return fr;
            LogDebug("[%s] fsync at out=%llu bytes", tag, (unsigned long long)written);
            next_fsync = written + opt.fsync_interval_bytes;
        }
    }

    auto fr = w.FsyncNow();
    if (!fr.is_ok()) return fr;

    EmitProgress(opt, tag, in_read ? *in_read : written, written, /*final=*/true);
    return Result::Ok();
}

} // namespace flash
