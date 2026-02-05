#include "flash/ota_bundle_reader.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

namespace flash {

OtaTarBundleReader::~OtaTarBundleReader() {
    if (ar_) {
        archive_read_free(ar_);
        ar_ = nullptr;
    }
}

Result OtaTarBundleReader::Open(IReader& src) {
    if (opened_) return Result::Fail(-1, "Bundle already opened");

    ar_ = archive_read_new();
    if (!ar_) return Result::Fail(-1, "archive_read_new failed");

    archive_read_support_format_tar(ar_);

    // Use archive_read_open with custom callbacks from IReader.
    // We'll use archive_read_open2 for read callback.
    struct Ctx {
        IReader* r = nullptr;
        std::vector<std::uint8_t> buf;
        explicit Ctx(IReader& rr) : r(&rr), buf(64 * 1024) {}
    };

    auto* ctx = new Ctx(src);

    auto read_cb = [](archive*, void* cd, const void** buff) -> la_ssize_t {
        auto* c = static_cast<Ctx*>(cd);
        const ssize_t n = c->r->Read(std::span<std::uint8_t>(c->buf.data(), c->buf.size()));
        if (n < 0) return -1;
        *buff = c->buf.data();
        return static_cast<la_ssize_t>(n); // 0 => EOF
    };

    auto close_cb = [](archive*, void* cd) -> int {
        auto* c = static_cast<Ctx*>(cd);
        delete c;
        return ARCHIVE_OK;
    };

    if (archive_read_open2(ar_, ctx, /*open*/nullptr, read_cb, /*skip*/nullptr, close_cb) != ARCHIVE_OK) {
        std::string em = archive_error_string(ar_) ? archive_error_string(ar_) : "unknown";
        archive_read_free(ar_);
        ar_ = nullptr;
        return Result::Fail(-1, "archive_read_open2 failed: " + em);
    }

    opened_ = true;
    return Result::Ok();
}

Result OtaTarBundleReader::Next(BundleEntryInfo& out, bool& eof) {
    eof = false;
    if (!opened_ || !ar_) return Result::Fail(-1, "Bundle not opened");

    // If previous entry not fully read, require caller to SkipCurrent()/read to EOF
    if (in_entry_) {
        return Result::Fail(-1, "Previous entry not finished (read to EOF or call SkipCurrent)");
    }

    while (true) {
        int r = archive_read_next_header(ar_, &cur_entry_);
        if (r == ARCHIVE_EOF) {
            eof = true;
            return Result::Ok();
        }
        if (r != ARCHIVE_OK) {
            std::string em = archive_error_string(ar_) ? archive_error_string(ar_) : "unknown";
            return Result::Fail(-1, "archive_read_next_header: " + em);
        }

        // Only regular files
        const auto t = archive_entry_filetype(cur_entry_);
        if (t != AE_IFREG) {
            // skip non-regular entries
            archive_read_data_skip(ar_);
            continue;
        }

        const char* name = archive_entry_pathname(cur_entry_);
        out.name = name ? std::string(name) : std::string();
        out.size = static_cast<std::uint64_t>(archive_entry_size(cur_entry_));

        in_entry_ = true;
        return Result::Ok();
    }
}

Result OtaTarBundleReader::SkipCurrent() {
    if (!in_entry_) return Result::Ok();
    if (archive_read_data_skip(ar_) != ARCHIVE_OK) {
        std::string em = archive_error_string(ar_) ? archive_error_string(ar_) : "unknown";
        return Result::Fail(-1, "archive_read_data_skip: " + em);
    }
    in_entry_ = false;
    return Result::Ok();
}

Result OtaTarBundleReader::ReadCurrentToString(std::string& out) {
    if (!in_entry_) return Result::Fail(-1, "No current entry");
    out.clear();

    std::vector<std::uint8_t> buf(64 * 1024);
    while (true) {
        const la_ssize_t n = archive_read_data(ar_, buf.data(), buf.size());
        if (n == 0) break;
        if (n < 0) {
            std::string em = archive_error_string(ar_) ? archive_error_string(ar_) : "unknown";
            return Result::Fail(-1, "archive_read_data: " + em);
        }
        out.append(reinterpret_cast<const char*>(buf.data()), static_cast<size_t>(n));
    }

    in_entry_ = false;
    return Result::Ok();
}

Result OtaTarBundleReader::OpenCurrentEntryReader(std::unique_ptr<IReader>& out_reader) {
    if (!in_entry_) return Result::Fail(-1, "No current entry");
    out_reader = std::make_unique<EntryReader>(this);
    return Result::Ok();
}

ssize_t OtaTarBundleReader::EntryReader::Read(std::span<std::uint8_t> out) {
    if (!parent_ || !parent_->in_entry_) return -1;
    const la_ssize_t n = archive_read_data(parent_->ar_, out.data(), out.size());
    if (n < 0) return -1;
    if (n == 0) {
        // entry finished
        parent_->in_entry_ = false;
        return 0;
    }
    return static_cast<ssize_t>(n);
}

std::optional<std::uint64_t> OtaTarBundleReader::EntryReader::TotalSize() const {
    if (!parent_ || !parent_->cur_entry_) return std::nullopt;
    const la_int64_t sz = archive_entry_size(parent_->cur_entry_);
    if (sz < 0) return std::nullopt;
    return static_cast<std::uint64_t>(sz);
}

} // namespace flash
