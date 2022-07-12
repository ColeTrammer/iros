#pragma once

#include <liim/container/concepts.h>
#include <liim/container/iterator/continuous_iterator.h>
namespace LIIM::Container::Producer {
template<typename T>
class Repeat {
public:
    explicit constexpr Repeat(size_t count, T value) : m_count(count), m_value(move(value)) {}

    class Iterator : public ContinuousIteratorAdapter<Iterator> {
    public:
        using ValueType = const T&;

        constexpr ValueType operator*() const { return m_value; }
        constexpr decltype(auto) operator->() const { return &m_value; }

    private:
        constexpr Iterator(size_t index, ValueType value) : ContinuousIteratorAdapter<Iterator>(index), m_value(value) {}

        friend Repeat;

        ValueType m_value;
    };

    constexpr Iterator begin() const { return Iterator(0, m_value); }
    constexpr Iterator end() const { return Iterator(m_count, m_value); }

    constexpr size_t size() const { return m_count; }

private:
    size_t m_count;
    T m_value;
};

template<typename T>
constexpr Repeat<T> repeat(size_t count, T value) {
    return Repeat(count, move(value));
}
}

using LIIM::Container::Producer::repeat;
