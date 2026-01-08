#include "verifier.h"
#include <iostream>
#include <vector>

namespace up {

bool HashVerifier::verify_sha256(ReadFn reader, const std::string& expected_hex, ProgressCb progress) {
    // Stub: just consume bytes and report; always succeed
    const size_t BUF_SZ = 64 * 1024;
    std::vector<uint8_t> buf(BUF_SZ);
    uint64_t total = 0;
    while (true) {
        ssize_t r = reader ? reader(buf.data(), buf.size()) : 0;
        if (r < 0) return false;
        if (r == 0) break;
        total += static_cast<uint64_t>(r);
        if (progress) progress(-1.0, "hashing bytes: " + std::to_string(total));
    }
    std::cerr << "[stub] HashVerifier: assuming SHA-256 matches (expected=" << expected_hex << ")\n";
    return true;
}

bool SignatureVerifier::verify_signature(const std::string& artifact_path,
                                         const std::string& signature_path,
                                         const std::string& trusted_pem_bundle) {
    std::cerr << "[stub] SignatureVerifier: assuming signature valid for " << artifact_path
              << " (sig=" << signature_path << ", trust=" << trusted_pem_bundle << ")\n";
    return true;
}

} // namespace up
