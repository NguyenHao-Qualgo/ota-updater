#include "flash/gzip_reader.hpp"
#include <stdexcept>

namespace flash {

GzipReader::GzipReader(std::unique_ptr<IReader> source) 
    : source_(std::move(source)), in_buffer_(16384) {
    strm_.zalloc = Z_NULL;
    strm_.zfree = Z_NULL;
    strm_.opaque = Z_NULL;
    strm_.avail_in = 0;
    strm_.next_in = Z_NULL;

    // 16 + MAX_WBITS tells zlib to expect a gzip header
    if (inflateInit2(&strm_, 16 + MAX_WBITS) != Z_OK) {
        throw std::runtime_error("Failed to initialize zlib inflate");
    }
}

GzipReader::~GzipReader() {
    inflateEnd(&strm_);
}

ssize_t GzipReader::Read(std::span<std::uint8_t> out) {
    if (eof_reached_) return 0;

    strm_.next_out = out.data();
    strm_.avail_out = static_cast<uInt>(out.size());

    while (strm_.avail_out > 0) {
        if (strm_.avail_in == 0) {
            ssize_t n = source_->Read(in_buffer_);
            if (n < 0) return -1; 
            if (n == 0) {
                // Source exhausted. If zlib hasn't finished, it's a truncated file.
                break; 
            }
            strm_.avail_in = static_cast<uInt>(n);
            strm_.next_in = in_buffer_.data();
        }

        int ret = inflate(&strm_, Z_NO_FLUSH);

        if (ret == Z_STREAM_END) {
            eof_reached_ = true;
            break;
        }

        // Z_BUF_ERROR is not fatal; it just means we need more input or output space.
        if (ret != Z_OK && ret != Z_BUF_ERROR) {
            return -1;
        }
        
        // If we made no progress and have no more input, stop to avoid infinite loop
        if (ret == Z_BUF_ERROR && strm_.avail_in == 0) {
            break;
        }
    }

    size_t produced = out.size() - strm_.avail_out;
    
    // Safety: If we produced 0 bytes but haven't reached Z_STREAM_END, 
    // and the source is dead, then it's an actual error/EOF.
    if (produced == 0 && !eof_reached_) {
        return 0; 
    }

    return static_cast<ssize_t>(produced);
}

} // namespace flash