#pragma once

#include <string>
#include <vector>
#include "flash/result.hpp"
#include "flash/manifest.hpp"
#include "flash/update_module.hpp"

namespace flash {

class ArchiveExtractor {
public:
    ArchiveExtractor() = default;

    /**
     * @brief High-level orchestration of the OTA update from a compressed file.
     * @param package_path Path to the .tar.gz or .tar file.
     */
    Result ProcessPackage(const std::string& package_path);

private:
    // Finds and parses the manifest.json inside the archive
    Result ExtractManifest(const std::string& path, Manifest& out_manifest);

    // Iterates the archive and triggers UpdateModule for each matching file
    Result ApplyUpdates(const std::string& path, const Manifest& manifest);
};

} // namespace flash