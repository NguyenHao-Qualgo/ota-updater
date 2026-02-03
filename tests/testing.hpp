#pragma once

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace testutil {

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        char tpl[] = "/tmp/flash_tool_tests_XXXXXX";
        char *p = ::mkdtemp(tpl);
        if (!p) {
            throw std::runtime_error("mkdtemp failed");
        }
        path_ = p;
    }

    ~TemporaryDirectory() {
        // Best-effort cleanup. Keep it simple: rely on "rm -rf".
        if (!path_.empty()) {
            std::string cmd = "rm -rf '" + path_ + "'";
            (void)::system(cmd.c_str());
        }
    }

    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;

    const std::string& Path() const { return path_; }

private:
    std::string path_;
};

} // namespace testutil
