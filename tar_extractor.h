#pragma once
#include "stream.h"
#include "types.h"
#include <string>

namespace up {

class TarExtractor {
public:
    void extract(ReadFn reader, const std::string& target_root, ProgressCb progress);
};

} // namespace up
