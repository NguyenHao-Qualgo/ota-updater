#pragma once

#include "flash/io.hpp"
#include "flash/result.hpp"
#include "flash/manifest.hpp"
#include <memory>

namespace flash {

class UpdateModule {
public:
    static Result Execute(const Component& comp, std::unique_ptr<IReader> source);

private:
    static Result InstallRaw(const Component& comp, IReader& reader);
    static Result InstallArchive(const Component& comp, IReader& reader);
    static Result InstallAtomicFile(const Component& comp, IReader& reader);

    // Internal helper to pipe data between reader and writer
    static Result InternalPipe(IReader& r, IWriter& w);
    
};

} // namespace flash