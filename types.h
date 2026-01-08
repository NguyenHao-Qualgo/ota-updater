#pragma once
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <chrono>

namespace up {

enum class ArtifactType { Raw, Ext4, Tar };
enum class Compression { None, Gzip, Zstd, Auto };
enum class InstallTarget { BlockDevice, MountPath };
enum class BootStrategy { AB, Single };
enum class Slot { A, B, Unknown };

struct Artifact {
    ArtifactType type;
    std::string source_path;       // path or URL
    Compression compression;
    InstallTarget target_kind;
    std::string target_path;       // /dev/mmcblk0p2 for block; /mnt/app for fs
    std::optional<std::string> sha256_hex;
    std::optional<std::string> signature_path; // detached .sig
    std::optional<size_t> expected_size;       // optional sanity check
    bool force_resize_ext4 = false;            // only for ext4 images
};

struct BootConfig {
    BootStrategy strategy = BootStrategy::AB;
    std::optional<std::string> inactive_slot_device; // e.g. /dev/mmcblk0p2
    std::optional<std::string> switch_cmd;           // e.g. fw_setenv boot_slot B
};

struct Manifest {
    std::string version;
    std::vector<Artifact> artifacts;
    BootConfig boot;
};

using ProgressCb = std::function<void(double /*0..100 or -1*/, const std::string&)>;

} // namespace up
