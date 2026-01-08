
// artifact_handler.cpp
#include "artifact_handler.h"
#include <iostream>

namespace up {

bool RawImageHandler::supports(const Artifact& /*a*/) const {
    // Stub: return true for raw artifacts; simplify here
    return true;
}

void RawImageHandler::install(const Artifact& a, InstallContext& ctx) {
    if (ctx.progress) ctx.progress(-1.0, "[stub] RawImageHandler installing: " + a.source_path);
    // No-op: real implementation should build reader→decompressor→writer pipeline
}

bool Ext4ImageHandler::supports(const Artifact& /*a*/) const {
    return true;
}

void Ext4ImageHandler::install(const Artifact& a, InstallContext& ctx) {
    if (ctx.progress) ctx.progress(-1.0, "[stub] Ext4ImageHandler installing: " + a.source_path);
    // No-op; may run resize2fs after raw write in real implementation
}

bool TarArchiveHandler::supports(const Artifact& /*a*/) const {
    return true;
}

void TarArchiveHandler::install(const Artifact& a, InstallContext& ctx) {
    if (ctx.progress) ctx.progress(-1.0, "[stub] TarArchiveHandler extracting: " + a.source_path);
    // No-op: real impl should stream-extract into a.target_path
}

} // namespace up