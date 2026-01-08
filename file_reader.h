#pragma once
#include "stream.h"
#include <string>

namespace up {

class FileReader : public IReader {
public:
    explicit FileReader(const std::string& path);
    ~FileReader() override;
    ssize_t read(uint8_t* buf, size_t cap) override;
private:
    int fd_;
};

class BlockDeviceWriter : public IWriter {
public:
    explicit BlockDeviceWriter(const std::string& dev_path, bool direct=false);
    void write_stream(ReadFn reader, ProgressCb progress) override;
private:
    std::string dev_;
    bool direct_;
};

class FileSystemWriter : public IWriter {
public:
    explicit FileSystemWriter(const std::string& mount_point);
    void write_stream(ReadFn reader, ProgressCb progress) override; // not used for TAR
    // TAR extraction is separate (see tar_extractor)
private:
    std::string root_;
};

} // namespace up
