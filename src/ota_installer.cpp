#include "flash/ota_installer.hpp"

#include "flash/file_reader.hpp"
#include "flash/logger.hpp"
#include "flash/manifest.hpp"
#include "flash/ota_bundle_reader.hpp"
#include "flash/update_module.hpp"

#include <memory>
#include <string>
#include <unordered_set>

namespace flash {

std::string NormalizeTarName(std::string s) {
    if (s.rfind("./", 0) == 0) s.erase(0, 2);
    return s;
}

static std::uint64_t ComputeOverallTotalFromFile(const std::string& input_path,
                                                 const Manifest& manifest) {
    if (input_path == "-") return 0;

    std::unordered_set<std::string> want;
    want.reserve(manifest.components.size());
    for (const auto& c : manifest.components) {
        if (!c.filename.empty()) want.insert(c.filename);
    }

    FileOrStdinReader input2;
    auto r = FileOrStdinReader::Open(input_path.c_str(), input2);
    if (!r.ok) {
        LogWarn("Pre-scan open failed, overall progress disabled: %s", r.msg.c_str());
        return 0;
    }

    OtaTarBundleReader bundle2;
    auto br = bundle2.Open(input2);
    if (!br.is_ok()) {
        LogWarn("Pre-scan bundle open failed, overall progress disabled: %s", br.message().c_str());
        return 0;
    }

    bool eof = false;
    BundleEntryInfo ent{};
    std::uint64_t total = 0;

    auto nr = bundle2.Next(ent, eof);
    if (!nr.is_ok() || eof) return 0;
    (void)bundle2.SkipCurrent();

    while (true) {
        auto rr = bundle2.Next(ent, eof);
        if (!rr.is_ok()) return 0;
        if (eof) break;

        const std::string name = NormalizeTarName(ent.name);
        if (want.find(name) != want.end()) {
            total += ent.size;
        }

        auto sk = bundle2.SkipCurrent();
        if (!sk.is_ok()) return 0;
    }

    return total;
}

static const Component* FindComponentByFilename(const Manifest& m, const std::string& filename) {
    for (const auto& c : m.components) {
        if (c.filename == filename) return &c;
    }
    return nullptr;
}

Result OtaInstaller::Run(const std::string& input_path) {
    // Open input
    FileOrStdinReader input;
    {
        auto r = FileOrStdinReader::Open(input_path.c_str(), input);
        if (!r.ok) return Result::Fail(-1, r.msg);
    }

    // Open bundle
    OtaTarBundleReader bundle;
    {
        auto r = bundle.Open(input);
        if (!r.is_ok()) return r;
    }

    // Read manifest (must be first entry)
    Manifest manifest{};
    {
        bool eof = false;
        BundleEntryInfo ent{};

        auto r = bundle.Next(ent, eof);
        if (!r.is_ok()) return r;
        if (eof) return Result::Fail(-1, "Empty ota.tar");

        const std::string name = NormalizeTarName(ent.name);
        if (name != "manifest.json") {
            return Result::Fail(-1, "manifest.json must be the first entry in ota.tar (got: " + name + ")");
        }

        std::string manifest_json;
        auto rr = bundle.ReadCurrentToString(manifest_json);
        if (!rr.is_ok()) return rr;

        auto exp = ManifestHandler::Parse(manifest_json);
        if (!exp) return Result::Fail(-1, "Manifest parse error: " + exp.error());
        manifest = *exp;

        LogInfo("Loaded manifest version=%s hw=%s components=%zu",
                manifest.version.c_str(),
                manifest.hw_compatibility.c_str(),
                manifest.components.size());
    }

    // Pre-scan overall total (only if input is a file path)
    const std::uint64_t overall_total = ComputeOverallTotalFromFile(input_path, manifest);
    if (overall_total > 0) {
        LogInfo("OTA overall total (bundle bytes) = %llu", (unsigned long long)overall_total);
    } else {
        LogInfo("OTA overall total unknown (stdin or pre-scan failed)");
    }

    // Process entries
    std::uint64_t overall_done_base = 0;

    bool eof = false;
    BundleEntryInfo ent{};
    while (true) {
        auto r = bundle.Next(ent, eof);
        if (!r.is_ok()) return r;
        if (eof) break;

        const std::string name = NormalizeTarName(ent.name);

        const Component* comp = FindComponentByFilename(manifest, name);
        if (!comp) {
            LogDebug("skip: %s", name.c_str());
            auto sk = bundle.SkipCurrent();
            if (!sk.is_ok()) return sk;
            continue;
        }

        LogInfo("Install: name=%s type=%s file=%s (entry=%llu bytes)",
                comp->name.c_str(), comp->type.c_str(), comp->filename.c_str(),
                (unsigned long long)ent.size);

        std::unique_ptr<IReader> entry_reader;
        auto er = bundle.OpenCurrentEntryReader(entry_reader);
        if (!er.is_ok()) return er;

        UpdateModule::Options uopt;
        uopt.progress = true;
        uopt.component_total_bytes = ent.size;          // per-component %
        uopt.overall_total_bytes = overall_total;       // overall % (0 => unknown)
        uopt.overall_done_base_bytes = overall_done_base;

        auto ur = UpdateModule::Execute(*comp, std::move(entry_reader), uopt);
        if (!ur.is_ok()) {
            return Result::Fail(-1, "component '" + comp->name + "' failed: " + ur.message());
        }

        // update overall base after success
        overall_done_base += ent.size;

        // Ensure entry fully consumed (in case installer stops early)
        auto sk = bundle.SkipCurrent();
        if (!sk.is_ok()) return sk;
    }

    LogInfo("OTA completed successfully");
    return Result::Ok();
}

} // namespace flash
