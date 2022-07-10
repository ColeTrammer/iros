#pragma once

#include <liim/container/concepts.h>
#include <liim/container/iterator/value_iterator.h>

namespace LIIM::Container::Producer {
template<typename T>
class Sequence : public ValueIteratorAdapter<Sequence<T>> {
public:
    explicit constexpr Sequence(T start, T step) : m_value(move(start)), m_step(move(step)) {}

    using ValueType = T;

    constexpr Option<T> next() {
        auto result = m_value;
        m_value += m_step;
        return result;
    }

private:
    T m_value;
    T m_step;
};

template<typename T>
constexpr Sequence<T> sequence(T start, T step = static_cast<T>(1)) {
    return Sequence<T>(move(start), move(step));
}
}

using LIIM::Container::Producer::sequence;
