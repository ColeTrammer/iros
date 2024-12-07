#pragma once

#include "di/function/container/function.h"
#include "dius/error.h"
#include "dius/memory_region.h"
#include "dius/runtime/tls.h"

#ifndef DIUS_USE_RUNTIME
#include "dius/posix/thread.h"
#else
namespace dius {
struct PlatformThread;
struct PlatformThreadDeleter;

struct PlatformThread : di::SelfPointer<PlatformThread> {
    static auto create(runtime::TlsInfo) -> di::Result<di::Box<PlatformThread, PlatformThreadDeleter>>;
    static auto current() -> PlatformThread&;

    PlatformThread() = default;

    auto id() const -> int { return thread_id; }
    auto join() -> di::Result<void>;

    auto thread_local_storage(usize tls_size) -> di::Span<byte> {
        return { reinterpret_cast<byte*>(this) - tls_size, tls_size };
    }

    int thread_id { 0 };
    di::Function<void()> entry;
    MemoryRegion stack;
};

struct PlatformThreadDeleter {
    void operator()(PlatformThread*) const;
};

auto spawn_thread(PlatformThread&) -> di::Result<void>;
}
#endif
