
#include <iostream>
#include "update_manager.h"
#include "types.h"  // ensure this is the correct header name in your project

int main(int argc, char** argv) {
    using namespace up;

    if (argc < 2) {
        std::cerr << "usage: ota_demo <manifest.json>\n";
        return 1;
    }

    UpdateManager mgr;

    mgr.set_progress_cb([](int pct, const std::string& msg) {
        if (pct >= 0) {
            std::cout << "progress: " << pct << "%\n";
        } else {
            std::cout << msg << "\n";
        }
    });

    mgr.install_from_manifest(argv[1]);
    return 0;
}
