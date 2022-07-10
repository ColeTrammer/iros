#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::Producer {
template<typename T>
class Repeat {
public:
    explicit constexpr Repeat(size_t count, T value) : m_count(count), m_value(move(value)) {}

    class Iterator {
    public:
        using ValueType = const T&;

        constexpr ValueType operator*() const { return m_value; }
        constexpr decltype(auto) operator->() const { return &m_value; }

        constexpr ValueType operator[](ssize_t index) const { return m_value; }

        constexpr Iterator& operator++() {
            ++m_index;
            return *this;
        }
        constexpr Iterator& operator--() {
            --m_index;
            return *this;
        }

        constexpr Iterator operator++(int) { return Iterator(m_index++, m_value); }
        constexpr Iterator operator--(int) { return Iterator(m_index--, m_value); }

        constexpr Iterator operator+(ssize_t n) const { return Iterator(m_index + n, m_value); }
        constexpr Iterator operator-(ssize_t n) const { return Iterator(m_index - n, m_value); }

        constexpr ssize_t operator-(const Iterator& other) const { return this->m_index - other.m_index; }

        constexpr Iterator& operator+=(ssize_t n) {
            m_index += n;
            return *this;
        }
        constexpr Iterator& operator-=(ssize_t n) {
            m_index -= n;
            return *this;
        }

        constexpr bool operator==(const Iterator& other) const { return this->m_index == other.m_index; }
        constexpr auto operator<=>(const Iterator& other) const { return this->m_index <=> other.m_index; }

    private:
        constexpr Iterator(size_t index, ValueType value) : m_index(index), m_value(value) {}

        friend Repeat;

        size_t m_index { 0 };
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
