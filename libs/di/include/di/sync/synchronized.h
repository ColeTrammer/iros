#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/copy_constructible.h>
#include <di/concepts/default_constructible.h>
#include <di/concepts/move_constructible.h>
#include <di/concepts/remove_cvref_same_as.h>
#include <di/function/invoke.h>
#include <di/meta/core.h>
#include <di/platform/prelude.h>
#include <di/sync/concepts/lock.h>
#include <di/sync/scoped_lock.h>
#include <di/util/guarded_reference.h>

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
    constexpr meta::InvokeResult<Fun, Value&> with_lock(Fun&& function) {
        auto guard = ScopedLock(m_lock);
        return function::invoke(util::forward<Fun>(function), m_value);
    }

    constexpr auto lock() { return LockedReference<Value, Lock>(m_value, m_lock); }

    constexpr Value& get_assuming_no_concurrent_accesses() { return m_value; }
    constexpr Value const& get_const_assuming_no_concurrent_mutations() const { return m_value; }

    Lock& get_lock() { return m_lock; }

private:
    Value m_value {};
    Lock m_lock {};
};
}
