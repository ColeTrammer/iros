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
    auto get_return_object() noexcept -> detail::DummyReturnObject<EnvPromise>;
    auto initial_suspend() noexcept -> SuspendAlways;
    auto final_suspend() noexcept -> SuspendAlways;
    void unhandled_exception() noexcept;
    void return_void() noexcept;

    auto unhandled_stopped() noexcept -> std::coroutine_handle<>;
    auto unhandled_error(vocab::Error) noexcept -> std::coroutine_handle<>;

    template<typename E>
    friend auto tag_invoke(types::Tag<get_env>, EnvPromise<E> const&) noexcept -> E const&;
};
}
