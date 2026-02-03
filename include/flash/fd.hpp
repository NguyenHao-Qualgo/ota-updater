#pragma once

namespace flash {

class Fd {
public:
    Fd() = default;
    explicit Fd(int fd);

    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;

    Fd(Fd&& other) noexcept;
    Fd& operator=(Fd&& other) noexcept;

    ~Fd();

    int Get() const;
    bool Valid() const;

    void Reset(int fd);
    void Close();

private:
    int fd_{-1};
};

} // namespace flash
