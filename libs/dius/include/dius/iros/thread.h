#pragma once

#include <dius/error.h>
#include <dius/runtime/tls.h>

namespace dius {
struct PlatformThread;
struct PlatformThreadDeleter;

struct SelfPointer {
    explicit SelfPointer() : self(static_cast<PlatformThread*>(static_cast<void*>(this))) {}

    PlatformThread* self { nullptr };
};

struct PlatformThread : SelfPointer {
    static di::Result<di::Box<PlatformThread, PlatformThreadDeleter>> create(runtime::TlsInfo);

    PlatformThread() = default;

    int id() const { return thread_id; }
    di::Result<void> join();

    di::Span<byte> thread_local_storage(usize tls_size) {
        return { reinterpret_cast<byte*>(this) - tls_size, tls_size };
    }

    int thread_id { 0 };
    int join_futex { 0 };
    di::Function<void()> entry;
};

struct PlatformThreadDeleter {
    void operator()(PlatformThread*) const;
};
}
