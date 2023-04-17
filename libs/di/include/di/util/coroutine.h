#pragma once

#include <di/concepts/language_void.h>
#include <di/types/prelude.h>

#ifndef DI_NO_USE_STD
#include <coroutine>
#else
namespace std {
template<typename, typename...>
struct coroutine_traits {};

template<typename R, typename... Args>
requires(requires { typename R::promise_type; })
struct coroutine_traits<R, Args...> {
    using promise_type = typename R::promise_type;
};

template<typename Promise = void>
struct coroutine_handle;

template<>
struct coroutine_handle<void> {
public:
    constexpr coroutine_handle() noexcept {}
    constexpr coroutine_handle(std::nullptr_t) noexcept {}

    constexpr coroutine_handle& operator=(std::nullptr_t) noexcept {
        m_frame_pointer = nullptr;
        return *this;
    }

    bool done() const noexcept { return __builtin_coro_done(m_frame_pointer); }
    constexpr explicit operator bool() const noexcept { return bool(m_frame_pointer); }

    void operator()() const { resume(); }
    void resume() const { return __builtin_coro_resume(m_frame_pointer); }

    void destroy() const { return __builtin_coro_destroy(m_frame_pointer); }

    constexpr void* address() const noexcept { return m_frame_pointer; }
    constexpr static coroutine_handle from_address(void* address) noexcept {
        coroutine_handle result;
        result.m_frame_pointer = address;
        return result;
    }

protected:
    void* m_frame_pointer { nullptr };
};

template<typename Promise>
requires(!di::concepts::LanguageVoid<Promise>)
struct coroutine_handle<Promise> {
public:
    constexpr coroutine_handle() noexcept {}
    constexpr coroutine_handle(std::nullptr_t) noexcept {}

    constexpr coroutine_handle& operator=(std::nullptr_t) noexcept {
        m_frame_pointer = nullptr;
        return *this;
    }

    constexpr operator coroutine_handle<>() const noexcept { return std::coroutine_handle<>::from_address(address()); }

    bool done() const noexcept { return __builtin_coro_done(m_frame_pointer); }
    constexpr explicit operator bool() const noexcept { return bool(m_frame_pointer); }

    void operator()() const { resume(); }
    void resume() const { return __builtin_coro_resume(m_frame_pointer); }

    void destroy() const { return __builtin_coro_destroy(m_frame_pointer); }

    Promise& promise() const {
        void* promise_pointer = __builtin_coro_promise(m_frame_pointer, alignof(Promise), false);
        return *static_cast<Promise*>(promise_pointer);
    }
    static coroutine_handle from_promise(Promise& promise) {
        coroutine_handle result;
        result.m_frame_pointer = __builtin_coro_promise((char*) &promise, alignof(Promise), true);
        return result;
    }

    constexpr void* address() const noexcept { return m_frame_pointer; }
    constexpr static coroutine_handle from_address(void* address) noexcept {
        coroutine_handle result;
        result.m_frame_pointer = address;
        return result;
    }

private:
    void* m_frame_pointer { nullptr };
};

constexpr bool operator==(std::coroutine_handle<> a, std::coroutine_handle<> b) noexcept {
    return a.address() == b.address();
}

constexpr std::strong_ordering operator<=>(std::coroutine_handle<> a, std::coroutine_handle<> b) noexcept {
    return a.address() <=> b.address();
}

struct noop_coroutine_promise {};

template<>
struct coroutine_handle<noop_coroutine_promise> {
private:
    struct Frame {
        constexpr static void noop() {}

        void (*resume)() = noop;
        void (*destroy)() = noop;
        struct noop_coroutine_promise promise;
    };
    static Frame static_frame;

public:
    constexpr operator coroutine_handle<>() const noexcept { return std::coroutine_handle<>::from_address(address()); }

    constexpr bool done() const noexcept { return false; }
    constexpr explicit operator bool() const noexcept { return true; }

    constexpr void operator()() const noexcept { resume(); }
    constexpr void resume() const noexcept {}

    constexpr void destroy() const noexcept {}

    constexpr noop_coroutine_promise& promise() const noexcept { return static_frame.promise; }

    constexpr void* address() const noexcept { return m_frame_pointer; }

private:
    friend coroutine_handle noop_coroutine() noexcept;

    explicit coroutine_handle() noexcept = default;

    void* m_frame_pointer { &static_frame };
};

using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;

inline noop_coroutine_handle::Frame noop_coroutine_handle::static_frame {};

inline noop_coroutine_handle noop_coroutine() noexcept {
    return noop_coroutine_handle();
}

struct suspend_never {
    constexpr bool await_ready() const noexcept { return true; }
    constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}
    constexpr void await_resume() const noexcept {}
};

struct suspend_always {
    constexpr bool await_ready() const noexcept { return false; }
    constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}
    constexpr void await_resume() const noexcept {}
};
}
#endif

namespace di {
template<typename Promise = void>
using CoroutineHandle = std::coroutine_handle<Promise>;

using NoopCoroutineHandle = std::noop_coroutine_handle;
using NoopCoroutinePromise = std::noop_coroutine_promise;

using SuspendAlways = std::suspend_always;
using SuspendNever = std::suspend_never;

using std::noop_coroutine;
}
