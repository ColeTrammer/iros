#pragma once

#include <dius/error.h>
#include <dius/runtime/tls.h>

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
    di::Atomic<int> join_word { 0 };
    byte* stack { nullptr };
    di::Function<void()> entry;
};

struct PlatformThreadDeleter {
    void operator()(PlatformThread*) const;
};
}
