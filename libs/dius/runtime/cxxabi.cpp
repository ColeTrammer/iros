#include <di/prelude.h>

namespace __cxxabiv1 {
extern "C" [[noreturn]] void __cxa_pure_virtual() {
    ASSERT(false);
    di::unreachable();
}

extern "C" int __cxa_atexit(void (*)(void*), void*, void*) {
    return 0;
}

__extension__ typedef int __guard __attribute__((mode(__DI__)));

extern "C" int __cxa_guard_acquire(__guard* guard) {
    __guard expected = 0;
    if (di::AtomicRef(*guard).compare_exchange_strong(expected, 1, di::MemoryOrder::AcquireRelease)) {
        return 1;
    }

    while (expected == 1) {
        expected = di::AtomicRef(*guard).load(di::MemoryOrder::Acquire);
    }

    return 0;
}

extern "C" void __cxa_guard_release(__guard* guard) {
    di::AtomicRef(*guard).store(2, di::MemoryOrder::Release);
}
}
