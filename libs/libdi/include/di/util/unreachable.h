#pragma once

namespace di::util {
[[noreturn]] inline void unreachable() {
    __builtin_unreachable();
}
}
