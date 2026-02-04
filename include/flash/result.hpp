#pragma once
#include <string>
#include <utility>

namespace flash {

struct Result {
    bool ok{true};
    int err{0};
    std::string msg;

    bool is_ok() const { return ok; } 
    std::string message() const { return msg; }

    static Result Ok() { return {}; }
    static Result Fail(int e, std::string m) { return {.ok=false, .err=e, .msg=std::move(m)}; }
};

} // namespace flash
