#pragma once

#include <di/prelude.h>
#include <pthread.h>

namespace dius {
struct PlatformThread : public di::Immovable {
    pthread_t id() const { return native_handle; }
    di::Result<void> join();

    pthread_t native_handle {};
    di::Function<void()> entry;
};
}
