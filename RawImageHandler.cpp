#include "UpdateManager.hpp"
#include <fstream>
#include <vector>

namespace up {

class RawImageHandler : public IArtifactHandler {
public:
    bool handle(const InstallStep& step, ProgressCb cb) override {
        std::ifstream src(step.source_file, std::ios::binary | std::ios::ate);
        if (!src.is_open()) {
            cb(-1, "Source not found: " + step.source_file);
            return false;
        }

        std::streamsize fileSize = src.tellg();
        src.seekg(0, std::ios::beg);

        std::ofstream dst(step.destination_device, std::ios::binary);
        if (!dst.is_open()) {
            cb(-1, "Destination access failed: " + step.destination_device);
            return false;
        }

        std::vector<char> buffer(1024 * 1024); 
        std::streamsize totalWritten = 0;

        while (totalWritten < fileSize) {
            src.read(buffer.data(), buffer.size());
            std::streamsize bytes = src.gcount();
            if (bytes <= 0) break;

            dst.write(buffer.data(), bytes);
            totalWritten += bytes;
            
            double progress = (fileSize > 0) ? (double)totalWritten / fileSize * 100.0 : 0;
            cb(progress, "Writing " + step.destination_device);
        }

        return true;
    }
};

} // namespace up