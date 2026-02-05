#include "flash/progress.hpp"
#include "flash/logger.hpp"

namespace flash {

class ConsoleProgress final : public IProgress {
public:
    explicit ConsoleProgress(std::uint64_t min_step_bytes = 4 * 1024 * 1024ULL)
        : min_step_(min_step_bytes) {}

    void OnProgress(const ProgressEvent& e) override {
        if (e.comp_done < next_) return;
        next_ = e.comp_done + min_step_;

        if (e.comp_total > 0) {
            int pct = (int)((e.comp_done * 100ULL) / e.comp_total);
            if (e.overall_total > 0) {
                int opct = (int)((e.overall_done * 100ULL) / e.overall_total);
                LogInfo("[OTA %d%%] [%.*s %d%%] %llu/%llu",
                        opct,
                        (int)e.component.size(), e.component.data(),
                        pct,
                        (unsigned long long)e.comp_done,
                        (unsigned long long)e.comp_total);
            } else {
                LogInfo("[%.*s %d%%] %llu/%llu",
                        (int)e.component.size(), e.component.data(),
                        pct,
                        (unsigned long long)e.comp_done,
                        (unsigned long long)e.comp_total);
            }
        } else {
            LogInfo("[%-.*s] %llu bytes",
                    (int)e.component.size(), e.component.data(),
                    (unsigned long long)e.comp_done);
        }
    }

private:
    std::uint64_t min_step_ = 0;
    std::uint64_t next_ = 0;
};

} // namespace flash
