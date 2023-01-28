#pragma once

#include <liim/function.h>

namespace LIIM {
class ScopeGuard {
public:
    template<typename Callback>
    ScopeGuard(Callback&& callback) : m_guard(move(callback)) {}

    ~ScopeGuard() {
        if (m_armed) {
            m_guard();
        }
    }

    void disarm() { m_armed = false; }

private:
    bool m_armed { true };
    Function<void()> m_guard;
};
}

using LIIM::ScopeGuard;
