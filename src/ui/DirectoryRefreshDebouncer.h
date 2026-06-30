#pragma once

#include <cstdint>

namespace finderx::ui {

inline constexpr std::uint64_t kDefaultDirectoryRefreshDebounceMs = 250;

class DirectoryRefreshDebouncer {
public:
    explicit DirectoryRefreshDebouncer(std::uint64_t delayMs) : delayMs_(delayMs) {}

    void request(std::uint64_t nowMs) {
        pending_ = true;
        dueMs_ = nowMs + delayMs_;
    }

    bool pending() const {
        return pending_;
    }

    void cancel() {
        pending_ = false;
        dueMs_ = 0;
    }

    bool consumeIfDue(std::uint64_t nowMs) {
        if (!pending_ || nowMs < dueMs_) {
            return false;
        }

        pending_ = false;
        return true;
    }

private:
    std::uint64_t delayMs_ = 0;
    std::uint64_t dueMs_ = 0;
    bool pending_ = false;
};

} // namespace finderx::ui
