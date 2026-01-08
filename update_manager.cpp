#include "update_manager.h"
#include "errors.h"

using namespace up;

UpdateManager::UpdateManager() {
    // default handlers
    register_handler(std::make_unique<RawImageHandler>());
    register_handler(std::make_unique<Ext4ImageHandler>());
    register_handler(std::make_unique<TarArchiveHandler>());
}

void UpdateManager::set_progress_cb(ProgressCb cb) { progress_ = std::move(cb); }

void UpdateManager::register_handler(std::unique_ptr<IArtifactHandler> h) {
    handlers_.push_back(std::move(h));
}

IArtifactHandler* UpdateManager::pick_handler(const Artifact& a) {
    for (auto& h : handlers_) if (h->supports(a)) return h.get();
    throw InstallError("No handler supports artifact type");
}

void UpdateManager::install_from_manifest(const std::string& manifest_path) {
    auto m = loader_.load_from_json(manifest_path);
    auto plan = planner_.build(m);

    auto info = slots_.detect(m.boot);

    InstallContext ctx{ m, info, progress_, &hash_verifier_, &sig_verifier_, &tar_extractor_ };

    journal_.save({ m.version, Step::Verifying, std::nullopt, 0, "" });

    run_plan(m, plan);

    journal_.save({ m.version, Step::PostInstallChecks, std::nullopt, 0, "" });
    HealthCheck hc; /* add checks */ hc.run_all(progress_);

    journal_.save({ m.version, Step::SwitchingBoot, std::nullopt, 0, "" });
    if (m.boot.strategy == BootStrategy::AB) {
        boot_.switch_to(info.inactive, m.boot);
    }

    journal_.save({ m.version, Step::Completed, std::nullopt, 0, "" });
}

void UpdateManager::run_plan(const Manifest& m, const InstallPlan& plan) {
    for (const auto& step : plan.steps) {
        if (progress_) progress_(-1, "Installing: " + step.name);

        InstallContext ctx{ m, slots_.detect(m.boot), progress_, &hash_verifier_, &sig_verifier_, &tar_extractor_ };

        verify_artifact(step.artifact, ctx);

        auto* handler = pick_handler(step.artifact);
        journal_.save({ m.version, Step::WritingArtifact, step.artifact.source_path, 0, step.artifact.target_path });
        handler->install(step.artifact, ctx);
    }
}

void UpdateManager::verify_artifact(const Artifact& a, InstallContext& ctx) {
    if (a.sha256_hex) {
        // Build reader pipeline same as install
        // (Re-create reader+decompressor to hash exact bytes to be written/extracted)
        // ...
        // if (!hash_verifier_.verify_sha256(reader, *a.sha256_hex, progress_)) throw VerifyError("SHA mismatch");
    }
    if (a.signature_path && !trust_pem_bundle_path_.empty()) {
        if (!sig_verifier_.verify_signature(a.source_path, *a.signature_path, trust_pem_bundle_path_)) {
            throw VerifyError("Signature verification failed");
        }
    }
}