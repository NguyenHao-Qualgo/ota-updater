#pragma once

#include "flash/result.hpp"
#include <string>

namespace flash {

class OtaInstaller {
public:
    Result Run(const std::string& input_path);

};

} // namespace flash
