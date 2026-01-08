#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

#include "types.h"

namespace up {

// Returns number of bytes read into buf (0=EOF, <0=error)
using ReadFn = std::function<ssize_t(uint8_t* buf, size_t cap)>;

class IReader {
public:
    virtual ~IReader() = default;
    virtual ssize_t read(uint8_t* buf, size_t cap) = 0;
};

class IDecompressor {
public:
    virtual ~IDecompressor() = default;
    // Wrap a read function to return decompressed bytes
    virtual ReadFn wrap(ReadFn src) = 0;
};

class IWriter {
public:
    virtual ~IWriter() = default;
    virtual void write_stream(ReadFn reader, ProgressCb progress) = 0;
};

} // namespace up
