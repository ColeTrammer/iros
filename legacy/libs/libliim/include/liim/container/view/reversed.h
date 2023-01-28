#pragma once

#include <liim/container/concepts.h>
#include <liim/container/iterator/reverse_iterator.h>

namespace LIIM::Container::View {
template<Container C>
class Reversed {
public:
    explicit constexpr Reversed(C container) : m_container(forward<C&&>(container)) {}

    constexpr auto begin() {
        if constexpr (requires { m_container.rbegin(); }) {
            return m_container.rbegin();
        } else {
            return ReverseIterator(m_container.end());
        }
    }

    constexpr auto end() {
        if constexpr (requires { m_container.rend(); }) {
            return m_container.rend();
        } else {
            return ReverseIterator(m_container.begin());
        }
    }

    constexpr auto size() const requires(SizedContainer<C>) { return m_container.size(); }

private:
    C m_container;
};

template<Container T>
constexpr Reversed<T> reversed(T&& container) {
    return Reversed<T>(forward<T>(container));
}
}

using LIIM::Container::View::reversed;
