#pragma once

#include <pthread.h>

#include "di/assert/assert_bool.h"
#include "di/meta/callable.h"
#include "di/sync/unique_lock.h"
#include "dius/mutex.h"

namespace dius {
class ConditionVariable {
public:
    ConditionVariable() { pthread_cond_init(&m_condition_variable, nullptr); }
    ConditionVariable(ConditionVariable const&) = delete;

    ~ConditionVariable() { pthread_cond_destroy(&m_condition_variable); }

    void operator=(ConditionVariable const&) = delete;

    void notify_one() {
        auto rv = pthread_cond_signal(&m_condition_variable);
        DI_ASSERT(rv == 0);
    }

    void notify_all() {
        auto rv = pthread_cond_broadcast(&m_condition_variable);
        DI_ASSERT(rv == 0);
    }

    void wait(di::UniqueLock<dius::Mutex>& lock) {
        DI_ASSERT(lock.owns_lock());
        auto rv = pthread_cond_wait(&m_condition_variable, lock.mutex()->native_handle());
        DI_ASSERT(rv == 0);
    }

    template<di::concepts::CallableTo<bool> Pred>
    void wait(di::UniqueLock<dius::Mutex>& lock, Pred predicate) {
        while (!predicate()) {
            wait(lock);
        }
    }

    auto native_handle() -> pthread_cond_t* { return &m_condition_variable; }

private:
    pthread_cond_t m_condition_variable;
};
}
