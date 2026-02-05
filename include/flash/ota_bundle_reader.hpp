#pragma once

#include "flash/io.hpp"
#include "flash/result.hpp"

#include <archive.h>
#include <archive_entry.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace flash {

struct BundleEntryInfo {
    std::string name;
    std::uint64_t size = 0;
};

class OtaTarBundleReader {
public:
    OtaTarBundleReader() = default;
    ~OtaTarBundleReader();

    OtaTarBundleReader(const OtaTarBundleReader&) = delete;
    OtaTarBundleReader& operator=(const OtaTarBundleReader&) = delete;

    // Open from an IReader (FileOrStdinReader).
    Result Open(IReader& src);

    // Move to next regular file entry.
    // Returns Ok + eof=true when end-of-archive.
    Result Next(BundleEntryInfo& out, bool& eof);

    // Read current entry fully to string (for manifest.json).
    Result ReadCurrentToString(std::string& out);

    // Open a reader that streams current entry data using archive_read_data().
    // Note: libarchive requires sequential access; you must finish reading (EOF) before calling Next().
    Result OpenCurrentEntryReader(std::unique_ptr<IReader>& out_reader);

    // Skip any remaining bytes of current entry.
    Result SkipCurrent();

private:
    static Result FailMsg(const std::string& msg) { return Result::Fail(-1, msg); }

    bool opened_ = false;
    struct archive* ar_ = nullptr;
    struct archive_entry* cur_entry_ = nullptr;
    bool in_entry_ = false;

    class EntryReader final : public IReader {
    public:
        explicit EntryReader(OtaTarBundleReader* parent) : parent_(parent) {}
        ssize_t Read(std::span<std::uint8_t> out) override;
        std::optional<std::uint64_t> TotalSize() const override;

    private:
        OtaTarBundleReader* parent_ = nullptr;
    };
};

} // namespace flash
