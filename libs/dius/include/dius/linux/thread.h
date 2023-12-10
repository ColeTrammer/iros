#pragma once

#include <di/function/container/function.h>
#include <dius/error.h>
#include <dius/memory_region.h>
#include <dius/runtime/tls.h>

#ifndef DIUS_USE_RUNTIME
#include <dius/posix/thread.h>
#else
namespace dius {
struct PlatformThread;
struct PlatformThreadDeleter;

struct PlatformThread : di::SelfPointer<PlatformThread> {
    static di::Result<di::Box<PlatformThread, PlatformThreadDeleter>> create(runtime::TlsInfo);
    static PlatformThread& current();

    PlatformThread() = default;

    int id() const { return thread_id; }
    di::Result<void> join();

    di::Span<byte> thread_local_storage(usize tls_size) {
        return { reinterpret_cast<byte*>(this) - tls_size, tls_size };
    }

    int thread_id { 0 };
    di::Function<void()> entry;
    MemoryRegion stack;
};

struct PlatformThreadDeleter {
    void operator()(PlatformThread*) const;
};

di::Result<void> spawn_thread(PlatformThread&);
}
#endif
