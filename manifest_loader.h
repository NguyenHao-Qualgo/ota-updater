#pragma once
#include "types.h"

namespace up {

class ManifestLoader {
public:
    Manifest load_from_json(const std::string& json_path);
    Manifest load_from_toml(const std::string& toml_path);
};

} // namespace up
