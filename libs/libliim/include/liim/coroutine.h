#pragma once

#include <assert.h>
#include <coroutine>
#include <liim/utilities.h>

namespace LIIM {
template<typename Promise = void>
using CoroutineHandle = std::coroutine_handle<Promise>;

struct SuspendAlways {
    bool await_ready() { return false; }
    void await_suspend(CoroutineHandle<>) {}
    void await_resume() {}
};

struct SuspendNever {
    bool await_ready() { return true; }
    void await_suspend(CoroutineHandle<>) {}
    void await_resume() {}
};
}

using LIIM::CoroutineHandle;
using LIIM::SuspendAlways;
using LIIM::SuspendNever;
