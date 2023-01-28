#pragma once

#include <liim/container/concepts.h>
#include <liim/container/iterator/move_iterator.h>

namespace LIIM::Container::View {
template<Container C>
class MoveElements {
public:
    explicit constexpr MoveElements(C&& container) : m_container(move(container)) {}

    constexpr auto begin() { return MoveIterator(m_container.begin()); }
    constexpr auto end() { return MoveIterator(m_container.end()); }

    constexpr auto size() const requires(SizedContainer<C>) { return m_container.size(); }

    constexpr C& base() { return m_container; }

private:
    C m_container;
};

template<Container T>
constexpr MoveElements<T> move_elements(T container) {
    return MoveElements<T>(move(container));
}
}

using LIIM::Container::View::move_elements;
