#pragma once

#include <di/concepts/copy_constructible.h>
#include <di/util/address_of.h>

namespace di::util {
template<concepts::CopyConstructible T>
[[gnu::noinline]] T black_box(T const& value) {
    // We want to produce an identical value from this function, without the compiler realizing it.
    // This is done by passing a pointer through inline assembly, since the compiler won't realize
    // the pointer points to the original object.
    T const volatile* result;
    asm volatile("mov %1, %0\n" : "=r"(result) : "r"(util::address_of(value)) : "memory");
    return *result;
}
}