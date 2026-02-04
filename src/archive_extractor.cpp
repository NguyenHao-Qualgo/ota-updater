#include "flash/archive_extractor.hpp"
#include <archive.h>
#include <archive_entry.h>
#include <iostream>
#include <sstream>

namespace flash {

/**
 * Custom IReader implementation that reads data directly 
 * from a specific entry inside a libarchive handle.
 */
class ArchiveEntryReader : public IReader {
public:
    ArchiveEntryReader(struct archive* a) : archive_(a) {}
    ssize_t Read(std::span<std::uint8_t> out) override {
        return archive_read_data(archive_, out.data(), out.size());
    }
private:
    struct archive* archive_;
};

Result ArchiveExtractor::ProcessPackage(const std::string& package_path) {
    Manifest manifest;

    // Pass 1: Find manifest.json
    auto res = ExtractManifest(package_path, manifest);
    if (!res.is_ok()) return res;

    // Pass 2: Process binary files
    return ApplyUpdates(package_path, manifest);
}

Result ArchiveExtractor::ExtractManifest(const std::string& path, Manifest& out_manifest) {
    struct archive* a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    if (archive_read_open_filename(a, path.c_str(), 10240) != ARCHIVE_OK) {
        return Result::Fail(-1, "Could not open archive: " + path);
    }

    struct archive_entry* entry;
    bool found = false;
    std::string json_content;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        std::string entry_name = archive_entry_pathname(entry);
        if (entry_name == "manifest.json") {
            size_t size = archive_entry_size(entry);
            json_content.resize(size);
            archive_read_data(a, &json_content[0], size);
            found = true;
            break;
        }
    }

    archive_read_free(a);

    if (!found) return Result::Fail(-1, "manifest.json not found in package");
    
    auto parse_result = ManifestHandler::Parse(json_content);

    if (!parse_result.has_value()) {
        return Result::Fail(-1, "Manifest Parse Error: " + parse_result.error());
    }

    out_manifest = std::move(parse_result.value());

    return Result::Ok();
}

Result ArchiveExtractor::ApplyUpdates(const std::string& path, const Manifest& manifest) {
    struct archive* a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    if (archive_read_open_filename(a, path.c_str(), 10240) != ARCHIVE_OK) {
        return Result::Fail(-1, "Could not open archive for second pass");
    }

    UpdateModule module;
    struct archive_entry* entry;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        std::string entry_name = archive_entry_pathname(entry);
        
        // Find if this archive file is listed in our manifest components
        for (const auto& comp : manifest.components) {
            if (comp.filename == entry_name) {
                ArchiveEntryReader reader(a);
                auto res = module.Execute(comp, std::make_unique<ArchiveEntryReader>(a));
                if (!res.is_ok()) {
                    archive_read_free(a);
                    return res;
                }
            }
        }
    }

    archive_read_free(a);
    return Result::Ok();
}

} // namespace flash