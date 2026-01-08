#pragma once
#include "types.h"
#include "manifest_loader.h"
#include "planner.h"
#include "artifact_handler.h"
#include "slot_manager.h"
#include "boot_control.h"
#include "journal.h"
#include "verifier.h"
#include "tar_extractor.h"
#include "health_check.h"

#include <memory>

namespace up {

class UpdateManager {
public:
    UpdateManager();

    void set_progress_cb(ProgressCb cb);

    // Entry point: install from manifest file
    void install_from_manifest(const std::string& manifest_path);

    // Dependency injection (optional setters if you don’t construct internally)
    void register_handler(std::unique_ptr<IArtifactHandler> handler);

    // Trusted certs bundle used by SignatureVerifier
    void set_trust_bundle_path(const std::string& pem_path);

private:
    ProgressCb progress_;
    ManifestLoader loader_;
    Planner planner_;
    SlotManager slots_;
    BootControl boot_;
    Journal journal_;
    HashVerifier hash_verifier_;
    SignatureVerifier sig_verifier_;
    TarExtractor tar_extractor_;
    std::vector<std::unique_ptr<IArtifactHandler>> handlers_;
    std::string trust_pem_bundle_path_;

    void run_plan(const Manifest& m, const InstallPlan& plan);
    void verify_artifact(const Artifact& a, InstallContext& ctx);
    IArtifactHandler* pick_handler(const Artifact& a);
};

} // namespace up
