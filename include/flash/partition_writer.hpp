#pragma once

#include "flash/fd.hpp"
#include "flash/io.hpp"
#include "flash/result.hpp"

#include <span>
#include <string>

namespace flash {

class PartitionWriter final : public IWriter {
public:
    static Result Open(std::string path, PartitionWriter &out);

    Result WriteAll(std::span<const std::uint8_t> in) override;
    Result FsyncNow() override;

private:
    std::string path_;
    Fd fd_;
};

} // namespace flash
