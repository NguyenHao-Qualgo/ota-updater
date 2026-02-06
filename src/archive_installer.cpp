#include "flash/archive_installer.hpp"
#include "flash/logger.hpp"
#include "flash/signals.hpp"

#include <archive.h>
#include <archive_entry.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <memory>
#include <sys/mount.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace flash {

namespace {

struct ArchiveReadDeleter {
    void operator()(archive* a) const {
        if (a) archive_read_free(a);
    }
};
struct ArchiveWriteDeleter {
    void operator()(archive* a) const {
        if (a) archive_write_free(a);
    }
};

class MountGuard {
public:
    MountGuard() = default;
    MountGuard(std::string mount_dir, bool mounted) : dir_(std::move(mount_dir)), mounted_(mounted) {}
    MountGuard(const MountGuard&) = delete;
    MountGuard& operator=(const MountGuard&) = delete;

    MountGuard(MountGuard&& o) noexcept : dir_(std::move(o.dir_)), mounted_(o.mounted_) { o.mounted_ = false; }
    MountGuard& operator=(MountGuard&& o) noexcept {
        if (this == &o) return *this;
        Cleanup();
        dir_ = std::move(o.dir_);
        mounted_ = o.mounted_;
        o.mounted_ = false;
        return *this;
    }

    ~MountGuard() { Cleanup(); }

    const std::string& Dir() const { return dir_; }
    void Release() { mounted_ = false; }

private:
    void Cleanup() {
        if (mounted_ && !dir_.empty()) {
            ::umount2(dir_.c_str(), 0);
        }
    }

    std::string dir_;
    bool mounted_ = false;
};

#include <fcntl.h>
#include <unistd.h>

class ChdirGuard {
public:
    explicit ChdirGuard(const std::string& new_dir) {
        old_fd_ = ::open(".", O_RDONLY | O_DIRECTORY);
        if (old_fd_ < 0) return;

        if (::chdir(new_dir.c_str()) != 0) {
            ::close(old_fd_);
            old_fd_ = -1;
        }
    }

    ~ChdirGuard() {
        if (old_fd_ >= 0) {
            (void)::fchdir(old_fd_);
            ::close(old_fd_);
        }
    }

    bool ok() const { return old_fd_ >= 0; }

private:
    int old_fd_ = -1;
};


static bool IsDevPath(std::string_view s) {
    return s.rfind("/dev/", 0) == 0;
}

static bool IsSafeRelativePath(const std::string& p) {
    if (p.empty()) return false;
    if (p.front() == '/') return false;
    if (p.find('\\') != std::string::npos) return false;

    std::string_view sv(p);
    while (!sv.empty()) {
        while (!sv.empty() && sv.front() == '/') sv.remove_prefix(1);
        auto pos = sv.find('/');
        auto seg = sv.substr(0, pos);
        if (seg == "..") return false;
        if (pos == std::string_view::npos) break;
        sv.remove_prefix(pos);
    }
    return true;
}

// Normalize tar path to a clean relative form:
// - strip leading "./"
// - strip leading "/" (avoid absolute)
// - collapse duplicate slashes
static std::string NormalizeTarPath(std::string s) {
    while (s.rfind("./", 0) == 0) s.erase(0, 2);
    while (!s.empty() && s.front() == '/') s.erase(0, 1);

    std::string out;
    out.reserve(s.size());
    bool prev_slash = false;
    for (char c : s) {
        const bool slash = (c == '/');
        if (slash && prev_slash) continue;
        out.push_back(c);
        prev_slash = slash;
    }
    return out;
}

static std::string JoinPath(const std::string& base, const std::string& rel) {
    if (base.empty()) return rel;
    if (rel.empty()) return base;
    std::string full = base;
    if (full.back() != '/') full.push_back('/');
    full += rel;
    return full;
}

struct ReaderCtx {
    IReader* r = nullptr;
    std::vector<std::uint8_t> buf;
    explicit ReaderCtx(IReader& in, size_t buf_sz = 64 * 1024) : r(&in), buf(buf_sz) {}
};

static la_ssize_t ReadCb(struct archive*, void* client_data, const void** out_buf) {
    if (flash::g_cancel.load(std::memory_order_relaxed)) {
        errno = EINTR;
        return -1;
    }
    auto* ctx = static_cast<ReaderCtx*>(client_data);
    const ssize_t n = ctx->r->Read(std::span<std::uint8_t>(ctx->buf.data(), ctx->buf.size()));
    if (n < 0) return -1;
    *out_buf = ctx->buf.data();
    return static_cast<la_ssize_t>(n); // 0 => EOF
}

static int CloseCb(struct archive*, void* client_data) {
    delete static_cast<ReaderCtx*>(client_data);
    return ARCHIVE_OK;
}

static std::string ArchiveErr(archive* a) {
    const char* s = a ? archive_error_string(a) : nullptr;
    return s ? std::string(s) : std::string("unknown");
}

} // namespace

ArchiveInstaller::ArchiveInstaller() : opt_() {
    opt_.mount_flags = MS_RELATIME;
}

ArchiveInstaller::ArchiveInstaller(Options opt) : opt_(std::move(opt)) {
    if (opt_.mount_flags == 0) opt_.mount_flags = MS_RELATIME;
}

Result ArchiveInstaller::InstallTarStreamToTarget(IReader& tar_stream, std::string_view install_to, std::string_view tag) {
    if (install_to.empty()) return Result::Fail(-1, "install_to is empty");

    if (IsDevPath(install_to)) {
        fs::path base = opt_.mount_base_dir.empty() ? "/mnt" : opt_.mount_base_dir;
        std::error_code ec;
        fs::create_directories(base, ec);

        std::string tmpl = (base / (opt_.mount_prefix + std::string("XXXXXX"))).string();
        std::vector<char> buf(tmpl.begin(), tmpl.end());
        buf.push_back('\0');

        char* created = ::mkdtemp(buf.data());
        if (!created) {
            const int err = errno;
            return Result::Fail(err, "mkdtemp failed: " + std::string(std::strerror(err)));
        }

        std::string mount_dir(created);
        LogInfo("[%.*s] mount %.*s -> %s",
                (int)tag.size(), tag.data(),
                (int)install_to.size(), install_to.data(),
                mount_dir.c_str());

        if (::mount(std::string(install_to).c_str(),
                    mount_dir.c_str(),
                    opt_.fs_type.c_str(),
                    opt_.mount_flags,
                    nullptr) != 0) {
            const int err = errno;
            return Result::Fail(err, "mount failed: " + std::string(std::strerror(err)));
        }

        MountGuard mg(mount_dir, true);

        auto r = ExtractTarStreamToDir(tar_stream, mg.Dir(), tag);
        if (!r.is_ok()) return r;

        if (::umount2(mg.Dir().c_str(), 0) != 0) {
            const int err = errno;
            return Result::Fail(err, "umount failed: " + std::string(std::strerror(err)));
        }
        mg.Release();

        LogInfo("[%.*s] archive install done", (int)tag.size(), tag.data());
        return Result::Ok();
    }

    // Case 2: folder path -> create if missing then extract
    {
        fs::path dst(install_to);
        std::error_code ec;
        fs::create_directories(dst, ec);
        if (ec) {
            return Result::Fail(-1, "create_directories failed: " + dst.string() + ": " + ec.message());
        }

        LogInfo("[%.*s] extract -> %s", (int)tag.size(), tag.data(), dst.string().c_str());
        auto r = ExtractTarStreamToDir(tar_stream, dst.string(), tag);
        if (!r.is_ok()) return r;

        LogInfo("[%.*s] archive install done", (int)tag.size(), tag.data());
        return Result::Ok();
    }
}

Result ArchiveInstaller::ExtractTarStreamToDir(IReader& tar_stream, const std::string& dst_dir, std::string_view tag) {
    std::unique_ptr<archive, ArchiveReadDeleter> ar(archive_read_new());
    if (!ar) return Result::Fail(-1, "archive_read_new failed");

    archive_read_support_filter_all(ar.get());
    archive_read_support_format_all(ar.get());

    auto* ctx = new ReaderCtx(tar_stream);
    if (archive_read_open2(ar.get(), ctx, nullptr, ReadCb, nullptr, CloseCb) != ARCHIVE_OK) {
        delete ctx;
        return Result::Fail(-1, "archive_read_open2: " + ArchiveErr(ar.get()));
    }

    std::unique_ptr<archive, ArchiveWriteDeleter> aw(archive_write_disk_new());
    if (!aw) return Result::Fail(-1, "archive_write_disk_new failed");

    int flags = 0;
    flags |= ARCHIVE_EXTRACT_UNLINK;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_TIME;

    flags |= ARCHIVE_EXTRACT_SECURE_NODOTDOT;
    flags |= ARCHIVE_EXTRACT_SECURE_SYMLINKS;
#if defined(ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS)
    flags |= ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS;  // now OK because we keep relative paths
#endif

    archive_write_disk_set_options(aw.get(), flags);
    archive_write_disk_set_standard_lookup(aw.get());

    // IMPORTANT: extract relative paths into dst_dir
    ChdirGuard cd(dst_dir);
    if (!cd.ok()) {
        return Result::Fail(errno, "chdir(dst_dir) failed: " + std::string(std::strerror(errno)));
    }

    std::uint64_t extracted = 0;
    std::uint64_t next_progress =
        (opt_.progress_interval_bytes ? opt_.progress_interval_bytes : (4ULL * 1024 * 1024));

    archive_entry* entry = nullptr;

    while (true) {
        const int r = archive_read_next_header(ar.get(), &entry);
        if (r == ARCHIVE_EOF) break;
        if (r != ARCHIVE_OK) return Result::Fail(-1, "archive_read_next_header: " + ArchiveErr(ar.get()));

        const char* p0 = archive_entry_pathname(entry);
        std::string rel = NormalizeTarPath(p0 ? std::string(p0) : std::string());

        if (rel.empty() || rel == ".") {
            (void)archive_read_data_skip(ar.get());
            continue;
        }
        if (opt_.safe_paths_only && !IsSafeRelativePath(rel)) {
            return Result::Fail(-1, "Unsafe path in archive: " + rel);
        }

        // Keep RELATIVE path (critical fix)
        archive_entry_set_pathname(entry, rel.c_str());

        // Hardlink target should also be relative
        if (const char* hl0 = archive_entry_hardlink(entry); hl0 && *hl0) {
            std::string rel_hl = NormalizeTarPath(std::string(hl0));
            if (!rel_hl.empty() && rel_hl != ".") {
                if (opt_.safe_paths_only && !IsSafeRelativePath(rel_hl)) {
                    return Result::Fail(-1, "Unsafe hardlink target in archive: " + rel_hl);
                }
                archive_entry_set_hardlink(entry, rel_hl.c_str());
            }
        }

        LogDebug("[%.*s] entry: %s/%s", (int)tag.size(), tag.data(), dst_dir.c_str(), rel.c_str());

        const int wh = archive_write_header(aw.get(), entry);
        if (wh != ARCHIVE_OK) return Result::Fail(-1, "archive_write_header: " + ArchiveErr(aw.get()));

        const void* buff = nullptr;
        size_t size = 0;
        la_int64_t offset = 0;

        while (true) {
            const int rr = archive_read_data_block(ar.get(), &buff, &size, &offset);
            if (rr == ARCHIVE_EOF) break;
            if (rr != ARCHIVE_OK) return Result::Fail(-1, "archive_read_data_block: " + ArchiveErr(ar.get()));

            const int ww = archive_write_data_block(aw.get(), buff, size, offset);
            if (ww != ARCHIVE_OK) return Result::Fail(-1, "archive_write_data_block: " + ArchiveErr(aw.get()));

            extracted += (std::uint64_t)size;
            if (opt_.progress && opt_.progress_interval_bytes > 0 && extracted >= next_progress) {
                LogInfo("[%.*s] extract progress: %llu bytes",
                        (int)tag.size(), tag.data(), (unsigned long long)extracted);
                next_progress = extracted + opt_.progress_interval_bytes;
            }
        }

        const int wf = archive_write_finish_entry(aw.get());
        if (wf != ARCHIVE_OK) return Result::Fail(-1, "archive_write_finish_entry: " + ArchiveErr(aw.get()));
    }

    return Result::Ok();
}


} // namespace flash
