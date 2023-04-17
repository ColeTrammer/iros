#pragma once

#include <di/function/container/prelude.h>
#include <di/util/prelude.h>
#include <di/vocab/error/prelude.h>
#include <pthread.h>

namespace dius {
struct PlatformThread : public di::Immovable {
    pthread_t id() const { return native_handle; }
    di::Result<void> join();

    pthread_t native_handle {};
    di::Function<void()> entry;
};

using PlatformThreadDeleter = di::DefaultDelete<PlatformThread>;
}
