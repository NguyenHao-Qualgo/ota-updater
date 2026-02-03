#include "flash/flasher.hpp"
#include "flash/signals.hpp"

#include <cerrno>
#include <cstring>
#include <string>
#include <vector>
#include <cstdio>
#include <cinttypes>

namespace flash {

Result Flasher::Run(IReader &reader, IWriter &writer, const FlashOptions &opt) {
    std::vector<std::uint8_t> buf(kBlockSize);

    const auto total_opt = reader.TotalSize();
    const bool show_progress = opt.progress && total_opt.has_value();

    std::uint64_t total_written = 0;
    std::uint64_t unsynced = 0;

    const std::uint64_t t0 = NowMs();
    std::uint64_t last_print = t0;

    while (!g_cancel.load(std::memory_order_relaxed)) {
        ssize_t n = reader.Read(buf);
        if (n == 0) {
            break; // EOF
        }
        if (n < 0) {
            return Result::Fail(errno, "Read failed (" + std::string(std::strerror(errno)) + ")");
        }

        auto wr = writer.WriteAll(std::span<const std::uint8_t>(buf.data(), static_cast<size_t>(n)));
        if (!wr.ok) {
            return wr;
        }

        total_written += static_cast<std::uint64_t>(n);

        if (opt.fsync_interval_bytes != 0) {
            unsynced += static_cast<std::uint64_t>(n);
            if (unsynced >= opt.fsync_interval_bytes) {
                auto fs = writer.FsyncNow();
                if (!fs.ok) {
                    return fs;
                }
                unsynced = 0;
            }
        }

        if (show_progress) {
            const auto now = NowMs();
            if (now - last_print >= 500) {
                double pct = 100.0 * (double) total_written / (double) *total_opt;
                double elapsed = (double) (now - t0) / 1000.0;
                std::fprintf(
                    stderr,
                    "\rProgress: %6.2f%% (%" PRIu64 "/%" PRIu64 " bytes)  Elapsed: %.1fs",
                    pct,
                    total_written,
                    *total_opt,
                    elapsed);
                last_print = now;
            }
        }
    }

    if (opt.fsync_interval_bytes != 0) {
        auto fs = writer.FsyncNow();
        if (!fs.ok) {
            return fs;
        }
    }

    const std::uint64_t t1 = NowMs();
    double sec = (double) (t1 - t0) / 1000.0;
    if (sec <= 0.0) {
        sec = 0.001;
    }
    double mib_s = ((double) total_written / (1024.0 * 1024.0)) / sec;

    if (show_progress && total_opt.has_value()) {
        double pct = 100.0 * (double) total_written / (double) *total_opt;
        if (pct > 100.0) {
            pct = 100.0;
        }
        std::fprintf(
            stderr,
            "\rProgress: %6.2f%% (%" PRIu64 "/%" PRIu64 " bytes)  Elapsed: %.1fs\n",
            pct,
            total_written,
            *total_opt,
            sec);
    }

    if (g_cancel.load(std::memory_order_relaxed)) {
        std::fprintf(
            stderr,
            "Canceled. Total bytes written: %" PRIu64 " | Time: %.2fs | Avg: %.2f MiB/s\n",
            total_written,
            sec,
            mib_s);
        return Result::Fail(ECANCELED, "Canceled by user");
    }

    std::fprintf(
        stderr,
        "Done. Total bytes written: %" PRIu64 " | Time: %.2fs | Avg: %.2f MiB/s\n",
        total_written,
        sec,
        mib_s);

    return Result::Ok();
}

} // namespace flash
