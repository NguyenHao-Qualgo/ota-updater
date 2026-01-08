#pragma once
#include "stream.h"
#include "types.h"
#include <string>

namespace up {

class HashVerifier {
public:
    bool verify_sha256(ReadFn reader, const std::string& expected_hex, ProgressCb progress);
};

class SignatureVerifier {
public:
    bool verify_signature(const std::string& artifact_path,
                          const std::string& signature_path,
                          const std::string& trusted_pem_bundle);
};

} // namespace up
