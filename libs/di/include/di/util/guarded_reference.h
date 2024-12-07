#pragma once

#include "di/meta/operations.h"
#include "di/util/forward.h"

namespace di::util {
template<typename T, typename Guard>
class GuardedReference {
public:
    template<typename... Args>
    requires(concepts::ConstructibleFrom<Guard, Args...>)
    constexpr explicit GuardedReference(T& value, Args&&... args)
        : m_guard(util::forward<Args>(args)...), m_value(&value) {}

    constexpr auto operator*() const -> T& { return *m_value; }
    constexpr auto operator->() const -> T* { return m_value; }

private:
    Guard m_guard;
    T* m_value;
};
}

namespace di {
using util::GuardedReference;
}
