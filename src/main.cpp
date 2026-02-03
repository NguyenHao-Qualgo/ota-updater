#define _FILE_OFFSET_BITS 64

#include "flash/file_reader.hpp"
#include "flash/flasher.hpp"
#include "flash/partition_writer.hpp"
#include "flash/signals.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>

namespace {

void PrintUsage(const char *argv) {
    std::fprintf(stderr,
        "Usage:\n"
        "  %s -i <input|-> -o </dev/partition> [-s <bytes>] [-f <bytes>] [--progress]\n"
        "\n"
        "Options:\n"
        "  -i, --input            Input file path or '-' for stdin\n"
        "  -o, --output           Output partition path (e.g. /dev/nvme0n1p1)\n"
        "  -f, --fsync-interval   Bytes between fsync() calls (default 1048576). 0 disables fsync\n"
        "      --progress         Print progress\n"
        "  -h, --help             Show this help\n",
        argv);
}

} // namespace

int main(int argc, char **argv) {
    flash::InstallSignalHandlers();

    const char *in = nullptr;
    const char *out = nullptr;

    flash::FlashOptions opt{};
    opt.fsync_interval_bytes = 1024 * 1024ULL;
    opt.progress = false;

    static option long_opts[] = {
        {"input", required_argument, nullptr, 'i'},
        {"output", required_argument, nullptr, 'o'},
        {"fsync-interval", required_argument, nullptr, 'f'},
        {"progress", no_argument, nullptr, 1000},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0},
    };

    int idx = 0;
    int c;
    while ((c = getopt_long(argc, argv, "hi:o:s:f:", long_opts, &idx)) != -1) {
        switch (c) {
            case 'h':
                PrintUsage(argv[0]);
                return 0;

            case 'i':
                in = optarg;
                break;

            case 'o':
                out = optarg;
                break;

            case 'f': {
                char *end = nullptr;
                unsigned long long v = std::strtoull(optarg, &end, 10);
                if (!end || *end != '\0') {
                    std::fprintf(stderr, "Invalid --fsync-interval: %s\n", optarg);
                    return 2;
                }
                opt.fsync_interval_bytes = static_cast<std::uint64_t>(v);
                break;
            }

            case 1000:
                opt.progress = true;
                break;

            default:
                PrintUsage(argv[0]);
                return 2;
        }
    }

    if (!in || !out) {
        PrintUsage(argv[0]);
        return 2;
    }

    flash::FileOrStdinReader reader;
    if (auto r = flash::FileOrStdinReader::Open(in, reader); !r.ok) {
        std::fprintf(stderr, "ERROR: %s\n", r.msg.c_str());
        return 1;
    }

    flash::PartitionWriter writer;
    if (auto w = flash::PartitionWriter::Open(out, writer); !w.ok) {
        std::fprintf(stderr, "ERROR: %s\n", w.msg.c_str());
        return 1;
    }

    flash::Flasher flasher;
    auto res = flasher.Run(reader, writer, opt);
    if (!res.ok) {
        std::fprintf(stderr, "ERROR: %s\n", res.msg.c_str());
        return 1;
    }

    return 0;
}
