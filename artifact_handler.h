#pragma once
#include "types.h"
#include "stream.h"
#include "verifier.h"
#include "tar_extractor.h"
#include "slot_manager.h"

#include <memory>

namespace up {

struct InstallContext {
    Manifest manifest;
    SlotInfo slots;
    ProgressCb progress;

    HashVerifier* hash_verifier = nullptr;
    SignatureVerifier* sig_verifier = nullptr;
    TarExtractor* tar_extractor = nullptr;
};

class IArtifactHandler {
public:
    virtual ~IArtifactHandler() = default;
    virtual bool supports(const Artifact& a) const = 0;
    virtual void install(const Artifact& a, InstallContext& ctx) = 0;
};

class RawImageHandler : public IArtifactHandler {
public:
    bool supports(const Artifact& a) const override;
    void install(const Artifact& a, InstallContext& ctx) override;
private:
    // helpers to build pipeline
    std::unique_ptr<IReader> make_reader(const Artifact& a);
    std::unique_ptr<IDecompressor> make_decompressor(const Artifact& a);
};

class Ext4ImageHandler : public IArtifactHandler {
public:
    bool supports(const Artifact& a) const override;
    void install(const Artifact& a, InstallContext& ctx) override;
private:
    void maybe_resize_ext4(const std::string& device, ProgressCb progress);
};

class TarArchiveHandler : public IArtifactHandler {
public:
    bool supports(const Artifact& a) const override;
    void install(const Artifact& a, InstallContext& ctx) override;
private:
    std::unique_ptr<IReader> make_reader(const Artifact& a);
    std::unique_ptr<IDecompressor> make_decompressor(const Artifact& a);
};

} // namespace up
