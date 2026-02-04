#include "flash/manifest.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace flash {

std::expected<Manifest, std::string> ManifestHandler::Parse(const std::string& jsonInput) {
    try {
        if (jsonInput.find_first_not_of(" \t\n\r") == std::string::npos) {
            return std::unexpected("Empty input");
        }

        auto j = json::parse(jsonInput);

        if (!j.is_object()) {
            return std::unexpected("JSON root must be an object");
        }

        Manifest m;
        m.version = j.value("version", "0.0.0");
        m.hw_compatibility = j.value("hw_compatibility", "");
        m.force_all = j.value("force_all", false);

        if (j.contains("components") && j["components"].is_array()) {
            for (const auto& item : j["components"]) {
                Component c;
                c.name = item.value("name", "");
                c.type = item.value("type", "");
                c.filename = item.value("filename", "");
                c.sha256 = item.value("sha256", "");
                c.version = item.value("version", "0.0.0");
                c.force = item.value("force", false);
                c.install_to = item.value("install_to", "");
                c.path = item.value("path", "");
                c.permissions = item.value("permissions", "0644");
                m.components.push_back(c);
            }
        } else if (j.contains("components") && !j["components"].is_array()) {
             return std::unexpected("'components' must be an array");
        }
        
        return m;
    } catch (const json::parse_error& e) {
        return std::unexpected(std::string("Syntax Error: ") + e.what());
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Internal Error: ") + e.what());
    }
}

int ManifestHandler::CompareVersions(const std::string& v1, const std::string& v2) {
    if (v1 == v2) return 0;

    auto v1_parts = v1 | std::views::split('.') | std::views::transform([](auto&& rng) {
        return std::string_view(rng);
    });
    auto v2_parts = v2 | std::views::split('.') | std::views::transform([](auto&& rng) {
        return std::string_view(rng);
    });

    auto it1 = v1_parts.begin();
    auto it2 = v2_parts.begin();

    // Iterate until both are exhausted
    while (it1 != v1_parts.end() || it2 != v2_parts.end()) {
        int val1 = 0, val2 = 0;

        if (it1 != v1_parts.end()) {
            auto sv = *it1;
            std::from_chars(sv.data(), sv.data() + sv.size(), val1);
            ++it1;
        }

        if (it2 != v2_parts.end()) {
            auto sv = *it2;
            std::from_chars(sv.data(), sv.data() + sv.size(), val2);
            ++it2;
        }

        if (val1 > val2) return 1;
        if (val1 < val2) return -1;
    }

    return 0;
}

bool ManifestHandler::ShouldUpdate(const Component& comp, const Manifest& manifest, const std::string& currentVersion) {
    if (manifest.force_all || comp.force) return true;

    return CompareVersions(comp.version, currentVersion) > 0;
}

} // namespace flash