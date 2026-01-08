#pragma once
#include "stream.h"

namespace up {

class NoDecompressor : public IDecompressor {
public:
    ReadFn wrap(ReadFn src) override { return src; }
};

class GzipDecompressor : public IDecompressor {
public:
    ReadFn wrap(ReadFn src) override;
};

class ZstdDecompressor : public IDecompressor {
public:
    ReadFn wrap(ReadFn src) override;
};
} // namespace up