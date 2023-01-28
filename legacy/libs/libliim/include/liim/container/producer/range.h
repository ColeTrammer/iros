#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::Producer {
template<typename T>
class Range {
public:
    explicit constexpr Range(T start, T end) : m_start(start), m_end(end) {}

    class Iterator {
    public:
        using ValueType = T;

        constexpr ValueType operator*() const { return m_value; }
        constexpr const ValueType* operator->() const { return &m_value; }

        constexpr ValueType operator[](ssize_t index) const { return m_value + index; }

        constexpr Iterator& operator++() {
            ++m_value;
            return *this;
        }
        constexpr Iterator& operator--() {
            --m_value;
            return *this;
        }

        constexpr Iterator operator++(int) { return Iterator(m_value++); }
        constexpr Iterator operator--(int) { return Iterator(m_value--); }

        constexpr Iterator operator+(ssize_t n) const { return Iterator(m_value + n); }
        constexpr Iterator operator-(ssize_t n) const { return Iterator(m_value - n); }

        constexpr ssize_t operator-(const Iterator& other) const { return this->m_value - other.m_value; }

        constexpr Iterator& operator+=(ssize_t n) {
            m_value += n;
            return *this;
        }
        constexpr Iterator& operator-=(ssize_t n) {
            m_value -= n;
            return *this;
        }

        constexpr bool operator==(const Iterator& other) const { return this->m_value == other.m_value; }
        constexpr auto operator<=>(const Iterator& other) const { return this->m_value <=> other.m_value; }

    private:
        constexpr explicit Iterator(T value) : m_value(move(value)) {}

        friend Range;

        T m_value;
    };

    constexpr Iterator begin() const { return Iterator(m_start); }
    constexpr Iterator end() const { return Iterator(m_end); }

    constexpr size_t size() const { return m_end - m_start; }

private:
    T m_start;
    T m_end;
};

template<typename T>
constexpr Range<T> range(T start, T end) {
    if (end < start) {
        return Range(start, start);
    }
    return Range(move(start), move(end));
}

template<typename T>
constexpr Range<T> range(T start) {
    return range(T {}, move(start));
}
}

using LIIM::Container::Producer::range;
