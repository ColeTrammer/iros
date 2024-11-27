#pragma once

#include <di/function/container/prelude.h>
#include <di/util/prelude.h>
#include <di/vocab/error/prelude.h>
#include <pthread.h>

namespace dius {
struct PlatformThread : public di::Immovable {
    auto id() const -> pthread_t { return native_handle; }
    auto join() -> di::Result<void>;

    pthread_t native_handle {};
    di::Function<void()> entry;
};

using PlatformThreadDeleter = di::DefaultDelete<PlatformThread>;
}
