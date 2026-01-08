#pragma once
#include <stdexcept>
#include <string>

namespace up {

struct InstallError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct VerifyError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct DeviceError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

} // namespace up
