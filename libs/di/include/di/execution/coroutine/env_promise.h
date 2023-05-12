#pragma once

#include <di/execution/coroutine/with_await_transform.h>
#include <di/execution/interface/get_env.h>
#include <di/function/tag_invoke.h>
#include <di/util/coroutine.h>
#include <di/vocab/error/error.h>

namespace di::execution {
namespace detail {
    template<typename Promise>
    struct DummyReturnObject {
        using promise_type = Promise;
    };
}

template<typename Env>
struct EnvPromise : WithAwaitTransform<EnvPromise<Env>> {
    detail::DummyReturnObject<EnvPromise> get_return_object() noexcept;
    SuspendAlways initial_suspend() noexcept;
    SuspendAlways final_suspend() noexcept;
    void unhandled_exception() noexcept;
    void return_void() noexcept;

    std::coroutine_handle<> unhandled_stopped() noexcept;
    std::coroutine_handle<> unhandled_error(vocab::Error) noexcept;

    friend Env const& tag_invoke(types::Tag<get_env>, EnvPromise const&) noexcept;
};
}
