#pragma once

#include "di/function/invoke.h"
#include "di/meta/core.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"
#include "di/platform/prelude.h"
#include "di/sync/concepts/lock.h"
#include "di/sync/scoped_lock.h"
#include "di/util/guarded_reference.h"

namespace di::sync {
template<typename Value, concepts::Lock Lock = DefaultLock>
using LockedReference = util::GuardedReference<Value, ScopedLock<Lock>>;

template<typename Value, concepts::Lock Lock = DefaultLock>
class Synchronized {
public:
    Synchronized()
    requires(concepts::DefaultConstructible<Value>)
    = default;

    template<typename U>
    requires(!concepts::SameAs<U, InPlace> && !concepts::RemoveCVRefSameAs<U, Synchronized> &&
             concepts::ConstructibleFrom<Value, U>)
    constexpr explicit Synchronized(U&& value) : m_value(util::forward<U>(value)) {}

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr explicit Synchronized(InPlace, Args&&... args) : m_value(util::forward<Args>(args)...) {}

    Synchronized(Synchronized&&) = delete;

    template<concepts::Invocable<Value&> Fun>
    constexpr auto with_lock(Fun&& function) -> meta::InvokeResult<Fun, Value&> {
        auto guard = ScopedLock(m_lock);
        return function::invoke(util::forward<Fun>(function), m_value);
    }

    constexpr auto lock() { return LockedReference<Value, Lock>(m_value, m_lock); }

    constexpr auto get_assuming_no_concurrent_accesses() -> Value& { return m_value; }
    constexpr auto get_const_assuming_no_concurrent_mutations() const -> Value const& { return m_value; }

    constexpr auto read() const -> Value
    requires(concepts::CopyConstructible<Value>)
    {
        auto guard = ScopedLock(m_lock);
        return m_value;
    }

    auto get_lock() -> Lock& { return m_lock; }

private:
    Value m_value {};
    Lock mutable m_lock {};
};

template<typename T>
Synchronized(T&&) -> Synchronized<T>;
}

namespace di {
using sync::Synchronized;
}
