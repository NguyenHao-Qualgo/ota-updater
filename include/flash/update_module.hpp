#pragma once

#include "flash/io.hpp"
#include "flash/result.hpp"
#include "flash/manifest.hpp"

#include <cstdint>
#include <memory>

namespace flash {

class UpdateModule {
public:
    struct Options {
        std::uint64_t fsync_interval_bytes = 1024 * 1024ULL;
        bool progress = true;
        std::uint64_t progress_interval_bytes = 4 * 1024 * 1024ULL;

        std::uint64_t component_total_bytes = 0;     // set to BundleEntryInfo.size
        std::uint64_t overall_total_bytes = 0;       // 0 => unknown
        std::uint64_t overall_done_base_bytes = 0;   // sum of completed component entry sizes before current one
    };

    static Result Execute(const Component& comp, std::unique_ptr<IReader> source) {
        return Execute(comp, std::move(source), Options{});
    }

    static Result Execute(const Component& comp, std::unique_ptr<IReader> source, const Options& opt);

private:
    static Result InstallRaw(const Component& comp, IReader& reader, const Options& opt, const char* tag, const std::uint64_t* in_read);
    static Result InstallArchive(const Component& comp, IReader& reader, const Options& opt, const char* tag, const std::uint64_t* in_read);
    static Result InstallAtomicFile(const Component& comp, IReader& reader, const Options& opt, const char* tag, const std::uint64_t* in_read);

    static Result InternalPipe(IReader& r, IWriter& w, const Options& opt, const char* tag, const std::uint64_t* in_read);
};

} // namespace flash
