#pragma once

#include "di/function/container/prelude.h"
#include "di/sync/prelude.h"
#include "di/util/prelude.h"
#include "di/vocab/pointer/prelude.h"
#include "dius/error.h"
#include "dius/runtime/tls.h"

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
    di::Atomic<int> join_word { 0 };
    byte* stack { nullptr };
    di::Function<void()> entry;
};

struct PlatformThreadDeleter {
    void operator()(PlatformThread*) const;
};
}
