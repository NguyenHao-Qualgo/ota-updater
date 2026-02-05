#define _FILE_OFFSET_BITS 64

#include "flash/logger.hpp"
#include "flash/ota_installer.hpp"
#include "flash/signals.hpp"

#include <getopt.h>

namespace {
void PrintUsage(const char* argv0) {
    flash::LogError("Usage: %s -i <ota.tar | -> [-v]", argv0);
}
} // namespace

int main(int argc, char** argv) {
    flash::InstallSignalHandlers();
    flash::Logger::Instance().SetLevel(flash::LogLevel::Info);

    const char* in = nullptr;

    static option long_opts[] = {
        {"input", required_argument, nullptr, 'i'},
        {"verbose", no_argument, nullptr, 'v'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0},
    };

    int idx = 0;
    int c;
    while ((c = getopt_long(argc, argv, "hi:v", long_opts, &idx)) != -1) {
        switch (c) {
            case 'h': PrintUsage(argv[0]); return 0;
            case 'i': in = optarg; break;
            case 'v': flash::Logger::Instance().SetLevel(flash::LogLevel::Debug); break;
            default:  PrintUsage(argv[0]); return 2;
        }
    }

    if (!in) { PrintUsage(argv[0]); return 2; }

    flash::OtaInstaller installer;
    auto r = installer.Run(in);
    if (!r.is_ok()) {
        flash::LogError("%s", r.message().c_str());
        return 1;
    }
    return 0;
}
