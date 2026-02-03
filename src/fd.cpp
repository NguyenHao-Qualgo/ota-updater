#include "flash/fd.hpp"

#include <unistd.h>

namespace flash {

Fd::Fd(int fd) : fd_(fd) {}

Fd::Fd(Fd &&other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

Fd &Fd::operator=(Fd &&other) noexcept {
    if (this != &other) {
        Close();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

Fd::~Fd() {
    Close();
}

int Fd::Get() const {
    return fd_;
}

bool Fd::Valid() const {
    return fd_ >= 0;
}

void Fd::Reset(int fd) {
    Close();
    fd_ = fd;
}

void Fd::Close() {
    if (fd_ >= 0 && fd_ != STDIN_FILENO) {
        ::close(fd_);
    }
    fd_ = -1;
}

} // namespace flash
