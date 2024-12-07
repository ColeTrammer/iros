#pragma once

#include "di/assert/prelude.h"
#include "di/function/prelude.h"
#include "di/meta/util.h"
#include "di/util/prelude.h"
#include "dius/config.h"
#include "dius/error.h"

#include DIUS_PLATFORM_PATH(thread.h)

namespace dius {
/// @brief Class representing a single thread of execution.
///
/// This class is modeled after the C++ standard library [std::thread](https://en.cppreference.com/w/cpp/thread/thread).
///
/// @warning All Thread objects must be joined before destruction, or the program will terminate.
class Thread {
private:
    static auto do_start(di::Function<void()>) -> di::Result<Thread>;

    explicit Thread(di::Box<PlatformThread, PlatformThreadDeleter> platform_thread)
        : m_platform_thread(di::move(platform_thread)) {}

public:
    using Id = di::ThreadId;

    template<di::concepts::MovableValue F, di::concepts::MovableValue... Args>
    requires(di::concepts::InvocableTo<F, void, Args...>)
    static auto create(F&& function, Args&&... args) -> di::Result<Thread> {
        auto copied_function = di::bind_front(di::forward<F>(function), di::forward<Args>(args)...);
        auto erased_function = di::make_function<void()>(di::move(copied_function));
        return do_start(di::move(erased_function));
    }

    Thread() = default;

    Thread(Thread const&) = delete;
    Thread(Thread&& other) : m_platform_thread(di::move(other.m_platform_thread)) {}

    ~Thread() { ASSERT(!joinable()); }

    auto operator=(Thread const&) -> Thread& = delete;
    auto operator=(Thread&& other) -> Thread& {
        ASSERT(!joinable());
        m_platform_thread = di::move(other.m_platform_thread);
        return *this;
    }

    [[nodiscard]] auto joinable() const -> bool { return Id() != get_id(); }
    [[nodiscard]] auto get_id() const -> Id { return m_platform_thread ? m_platform_thread->id() : Id(); }

    /// @brief Wait for the spawned thread's execution to terminate
    ///
    /// @warning It is undefined behavior to call this method concurrently from different threads.
    ///
    /// @return Returns an error if the operation failed.
    auto join() -> di::Result<void> {
        if (!m_platform_thread) {
            return di::Unexpected(PosixError::InvalidArgument);
        }
        auto guard = di::ScopeExit([&] {
            m_platform_thread.reset();
        });
        return m_platform_thread->join();
    }

private:
    friend void tag_invoke(di::Tag<di::swap>, Thread& a, Thread& b) {
        di::swap(a.m_platform_thread, b.m_platform_thread);
    }

    di::Box<PlatformThread, PlatformThreadDeleter> m_platform_thread;
};
}
