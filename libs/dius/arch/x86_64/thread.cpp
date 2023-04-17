#include <dius/thread.h>

namespace di::platform {
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
