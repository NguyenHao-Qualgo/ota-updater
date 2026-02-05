#pragma once

#include "flash/io.hpp"
#include "flash/result.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace flash {

class ArchiveInstaller {
public:
    struct Options {
        bool progress = true;
        std::uint64_t progress_interval_bytes = 4 * 1024 * 1024ULL;
        bool safe_paths_only = true;

        std::string mount_base_dir = "/mnt";
        std::string mount_prefix = "ota-";
        std::string fs_type = "ext4";

        // Keep header portable: do NOT reference MS_* macros here.
        unsigned long mount_flags = 0;
    };

    ArchiveInstaller();                 // default
    explicit ArchiveInstaller(Options opt);

    Result InstallTarStreamToTarget(IReader& tar_stream, std::string_view install_to, std::string_view tag);

private:
    Options opt_{};

    Result ExtractTarStreamToDir(IReader& tar_stream, const std::string& dst_dir, std::string_view tag);
};

} // namespace flash
