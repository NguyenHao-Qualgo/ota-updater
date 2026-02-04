#pragma once

#include <string>
#include <vector>
#include <expected>

namespace flash {

struct Component {
    std::string name;
    std::string type;
    std::string filename;
    std::string sha256;
    std::string version;
    bool force = false;
    
    std::string install_to;   
    std::string path;         
    std::string permissions = "0644";
};

struct Manifest {
    std::string version;
    std::string hw_compatibility;
    bool force_all = false;
    std::vector<Component> components;
};

class ManifestHandler {
public:
    static std::expected<Manifest, std::string> Parse(const std::string& jsonInput);

    static bool ShouldUpdate(const Component& comp, const Manifest& manifest, const std::string& currentVersion);
    
    static int CompareVersions(const std::string& v1, const std::string& v2);
};

} // namespace flash