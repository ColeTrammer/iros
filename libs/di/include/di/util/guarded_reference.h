#pragma once

#include <di/concepts/constructible_from.h>
#include <di/util/forward.h>

namespace di::util {
template<typename T, typename Guard>
class GuardedReference {
public:
    template<typename... Args>
    requires(concepts::ConstructibleFrom<Guard, Args...>)
    constexpr explicit GuardedReference(T& value, Args&&... args)
        : m_guard(util::forward<Args>(args)...), m_value(&value) {}

    constexpr T& operator*() const { return *m_value; }
    constexpr T* operator->() const { return m_value; }

private:
    Guard m_guard;
    T* m_value;
};
}
