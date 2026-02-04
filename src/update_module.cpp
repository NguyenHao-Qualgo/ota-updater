#include "flash/update_module.hpp"
#include "flash/gzip_reader.hpp"
#include "flash/partition_writer.hpp"
#include <algorithm>
#include <cstring>
#include <sys/stat.h>

namespace flash {

Result UpdateModule::Execute(const Component& comp, std::unique_ptr<IReader> source) {
    if (!source) return Result::Fail(-1, "Null source reader");

    std::unique_ptr<IReader> effective_reader = std::move(source);

    if (comp.filename.length() >= 3 && 
        comp.filename.compare(comp.filename.length() - 3, 3, ".gz") == 0) {
        try {
            effective_reader = std::make_unique<GzipReader>(std::move(effective_reader));
        } catch (const std::exception& e) {
            return Result::Fail(-1, std::string("Gzip init failed: ") + e.what());
        }
    }

    // 2. Routing to Handlers
    if (comp.type == "raw") {
        return InstallRaw(comp, *effective_reader);
    } else if (comp.type == "archive") {
        return InstallArchive(comp, *effective_reader);
    } else if (comp.type == "file") {
        return InstallAtomicFile(comp, *effective_reader);
    }

    return Result::Fail(-1, "Unsupported component type: " + comp.type);
}

Result UpdateModule::InstallRaw(const Component& comp, IReader& reader) {
    PartitionWriter writer;
    auto res = PartitionWriter::Open(comp.install_to, writer);
    if (!res.is_ok()) return res;

    return InternalPipe(reader, writer);
}

Result UpdateModule::InstallArchive(const Component& comp, IReader& reader) {

    return Result::Fail(-1, "Archive extraction not yet implemented");
}

Result UpdateModule::InstallAtomicFile(const Component& comp, IReader& reader) {
    if (comp.path.empty()) {
        return Result::Fail(-1, "File path is empty for component: " + comp.name);
    }

    // 1. Define temporary path (must be on the same partition for atomic rename)
    std::string tmp_path = comp.path + ".tmp";
    
    // 2. Open PartitionWriter as a standard file writer
    PartitionWriter writer;
    auto res = PartitionWriter::Open(tmp_path, writer);
    if (!res.is_ok()) return res;

    // 3. Pipe the data
    res = InternalPipe(reader, writer);
    if (!res.is_ok()) {
        ::unlink(tmp_path.c_str()); // Cleanup on failure
        return res;
    }

    // 4. Atomic Swap
    if (::rename(tmp_path.c_str(), comp.path.c_str()) != 0) {
        int err = errno;
        ::unlink(tmp_path.c_str());
        return Result::Fail(err, "Atomic rename failed: " + std::string(std::strerror(err)));
    }

    // 5. Set Permissions (Optional but recommended for 'file' type)
    if (!comp.permissions.empty()) {
        mode_t mode = static_cast<mode_t>(std::stoul(comp.permissions, nullptr, 8));
        ::chmod(comp.path.c_str(), mode);
    }

    return Result::Ok();
}
Result UpdateModule::InternalPipe(IReader& r, IWriter& w) {
    std::vector<std::uint8_t> buffer(32768); // 32KB buffer for performance
    while (true) {
        ssize_t n = r.Read(buffer);
        if (n == 0) break; // Success
        if (n < 0) return Result::Fail(errno, "Read failed during pipe");

        auto res = w.WriteAll({buffer.data(), static_cast<size_t>(n)});
        if (!res.is_ok()) return res;
    }
    return w.FsyncNow();
}

} // namespace flash