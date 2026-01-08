#include "journal.h"
#include <fstream>
#include <iostream>

namespace up {

Journal::Journal(const std::string& path) : path_(path) {}

UpdateState Journal::load() {
    // Stub: return default state
    UpdateState s;
    return s;
}

void Journal::save(const UpdateState& s) {
    std::ofstream ofs(path_, std::ios::trunc);
    if (!ofs) {
        std::cerr << "[stub] Journal: cannot open " << path_ << "\n";
        return;
    }
    ofs << "version=" << s.version << "\n";
    ofs << "step=" << static_cast<int>(s.current_step) << "\n";
    ofs << "artifact=" << (s.current_artifact ? *s.current_artifact : "") << "\n";
    ofs << "bytes_written=" << s.bytes_written << "\n";
    ofs << "target=" << s.target << "\n";
}

void Journal::clear() {
    std::ofstream ofs(path_, std::ios::trunc);
}

} // namespace up
