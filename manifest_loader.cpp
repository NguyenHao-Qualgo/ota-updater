#include "manifest_loader.h"
#include <iostream>

namespace up {

Manifest ManifestLoader::load_from_json(const std::string& json_path) {
    std::cerr << "[stub] ManifestLoader: loading " << json_path << " (stub)\n";
    Manifest m;
    m.version = "0.0.0";
    return m;
}

} // namespace up
