#pragma once

#include <di/chrono/duration/duration.h>

namespace di::chrono {
using Nanoseconds = Duration<i64, math::Nano>;
using Microseconds = Duration<i64, math::Micro>;
using Milliseconds = Duration<i64, math::Milli>;
using Seconds = Duration<i64>;
using Minutes = Duration<i64, math::Ratio<60>>;
using Hours = Duration<i64, math::Ratio<3600>>;
using Days = Duration<i32, math::Ratio<86400>>;
using Weeks = Duration<i32, math::Ratio<604800>>;
using Months = Duration<i32, math::Ratio<2629746>>;
using Years = Duration<i32, math::Ratio<31556952>>;
}

namespace di {
inline namespace literals {
    inline namespace chrono_literals {
        constexpr auto operator""_h(unsigned long long value) {
            return chrono::Hours { value };
        }

        constexpr auto operator""_min(unsigned long long value) {
            return chrono::Minutes { value };
        }

        constexpr auto operator""_s(unsigned long long value) {
            return chrono::Seconds { value };
        }

        constexpr auto operator""_ms(unsigned long long value) {
            return chrono::Milliseconds { value };
        }

        constexpr auto operator""_us(unsigned long long value) {
            return chrono::Microseconds { value };
        }

        constexpr auto operator""_ns(unsigned long long value) {
            return chrono::Nanoseconds { value };
        }
    }
}
}