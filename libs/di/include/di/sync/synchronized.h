#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/copy_constructible.h>
#include <di/concepts/default_constructible.h>
#include <di/concepts/move_constructible.h>
#include <di/function/invoke.h>
#include <di/platform/prelude.h>
#include <di/sync/concepts/lock.h>
#include <di/sync/scoped_lock.h>

namespace di::sync {
template<typename Value, concepts::Lock Lock = DefaultLock>
class Synchronized {
public:
    Synchronized()
    requires(concepts::DefaultConstructible<Value>)
    = default;

    constexpr explicit Synchronized(Value const& value)
    requires(concepts::CopyConstructible<Value>)
        : m_value(value) {}

    constexpr explicit Synchronized(Value&& value)
    requires(concepts::MoveConstructible<Value>)
        : m_value(util::move(value)) {}

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr explicit Synchronized(InPlace, Args&&... args) : m_value(util::forward<Args>(args)...) {}

    Synchronized(Synchronized&&) = delete;

    template<concepts::Invocable<Value&> Fun>
    constexpr meta::InvokeResult<Fun, Value&> with_lock(Fun&& function) {
        auto guard = ScopedLock(m_lock);
        return function::invoke(util::forward<Fun>(function), m_value);
    }

private:
    Value m_value {};
    Lock m_lock {};
};
}