#include <di/util/prelude.h>
#include <dius/thread.h>

namespace di::platform {
struct TlsIndex {
    unsigned long ti_module;
    unsigned long ti_offset;
};

extern "C" void* __tls_get_addr(TlsIndex*) {
    ASSERT(false);
    di::unreachable();
}

ThreadId get_current_thread_id() {
    return dius::PlatformThread::current().id();
}
}

namespace dius {
PlatformThread& PlatformThread::current() {
    PlatformThread* result;
    asm volatile("mov %%fs:0, %0" : "=r"(result));
    return *result;
}
}
