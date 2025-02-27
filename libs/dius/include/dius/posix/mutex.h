#pragma once

#include <pthread.h>

#include "di/assert/assert_bool.h"

namespace dius {
class Mutex {
public:
    Mutex() { pthread_mutex_init(&m_mutex, nullptr); }
    Mutex(Mutex const&) = delete;

    ~Mutex() { pthread_mutex_destroy(&m_mutex); }

    void operator=(Mutex const&) = delete;

    void lock() {
        auto rv = pthread_mutex_lock(&m_mutex);
        DI_ASSERT(rv == 0);
    }

    auto try_lock() -> bool {
        auto rv = pthread_mutex_trylock(&m_mutex);
        return rv == 0;
    }

    void unlock() {
        auto rv = pthread_mutex_unlock(&m_mutex);
        DI_ASSERT(rv == 0);
    }

    auto native_handle() -> pthread_mutex_t* { return &m_mutex; }

private:
    pthread_mutex_t m_mutex;
};
}
