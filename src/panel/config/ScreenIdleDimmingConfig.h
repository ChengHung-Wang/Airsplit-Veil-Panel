#pragma once

#include <stdint.h>

namespace ScreenIdleDimmingConfig {
    constexpr bool kEnabled = true;
    constexpr uint32_t kIdleDelayMs = 5000;
    constexpr uint8_t kDimPercent = 50;
    constexpr uint32_t kTransitionDurationMs = 150;
}
