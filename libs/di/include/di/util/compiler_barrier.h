#pragma once

namespace di::util {
/// @brief Force to optimizer to reload memory addresses
///
/// In certain cases, the compiler is able to cache the result of a read from a memory address. Inserting a compiler
/// barrier will prevent this, and force the compiler to reload the value.
///
/// @warning In most cases, it will be more clear to use a di::Atomic<> variable with relaxed memory ordering when this
///          behavior is desired.
constexpr void compiler_barrier() {
    if consteval {
        ;
    } else {
        asm volatile("" ::: "memory");
    }
}
}
