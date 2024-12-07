#include "dius/thread.h"

#include "di/util/prelude.h"

namespace di::platform {
struct TlsIndex {
    unsigned long ti_module;
    unsigned long ti_offset;
};

extern "C" auto __tls_get_addr(TlsIndex*) -> void* {
    ASSERT(false);
    di::unreachable();
}

auto get_current_thread_id() -> ThreadId {
    return dius::PlatformThread::current().id();
}
}

namespace dius {
auto PlatformThread::current() -> PlatformThread& {
    PlatformThread* result;
    asm volatile("mov %%fs:0, %0" : "=r"(result));
    return *result;
}
}
