#pragma once

#include <liim/initializer_list.h>
#include <liim/utilities.h>

namespace LIIM {
template<typename T>
struct IteratorTraits {
    using ValueType = T::ValueType;
};

template<typename T>
struct IteratorTraits<T*> {
    using ValueType = T&;
};

template<typename T>
concept Iterator = requires(T iterator, T other) {
    { *iterator } -> SameAs<typename IteratorTraits<T>::ValueType>;
    { ++iterator } -> SameAs<T&>;
    { iterator++ } -> SameAs<T>;
    { iterator == other } -> SameAs<bool>;
    { iterator != other } -> SameAs<bool>;
};

template<typename T>
concept Container = requires(T container) {
    { container.begin() } -> Iterator;
    { container.end() } -> Iterator;
};

template<typename T>
concept SizedContainer = Container<T> && requires(T container) {
    { container.size() } -> SameAs<size_t>;
};

template<Iterator Iter>
class ReverseIterator {
public:
    constexpr explicit ReverseIterator(Iter iter) : m_iterator(move(iter)) {}

    constexpr Iter base() const { return m_iterator; }

    constexpr decltype(auto) operator*() const { return m_iterator[-1]; }
    constexpr decltype(auto) operator->() const { return &m_iterator[-1]; }

    constexpr decltype(auto) operator[](ssize_t index) const { return m_iterator[-index]; }

    constexpr ReverseIterator& operator++() {
        --m_iterator;
        return *this;
    }
    constexpr ReverseIterator& operator--() {
        ++m_iterator;
        return *this;
    }

    constexpr ReverseIterator operator++(int) const { return ReverseIterator(m_iterator--); }
    constexpr ReverseIterator operator--(int) const { return ReverseIterator(m_iterator++); }

    constexpr ReverseIterator operator+(ssize_t n) const { return ReverseIterator(m_iterator - n); }
    constexpr ReverseIterator operator-(ssize_t n) const { return ReverseIterator(m_iterator + n); }

    constexpr ssize_t operator-(const ReverseIterator& other) const { return other.base() - *this.base(); }

    constexpr ReverseIterator& operator+=(ssize_t n) {
        m_iterator -= n;
        return *this;
    }
    constexpr ReverseIterator& operator-=(ssize_t n) {
        m_iterator += n;
        return *this;
    }

    constexpr bool operator==(const ReverseIterator& other) const { return this->m_iterator == other.m_iterator; }
    constexpr bool operator!=(const ReverseIterator& other) const { return this->m_iterator != other.m_iterator; }
    constexpr bool operator<=(const ReverseIterator& other) const { return this->m_iterator >= other.m_iterator; }
    constexpr bool operator<(const ReverseIterator& other) const { return this->m_iterator > other.m_iterator; }
    constexpr bool operator>=(const ReverseIterator& other) const { return this->m_iterator <= other.m_iterator; }
    constexpr bool operator>(const ReverseIterator& other) const { return this->m_iterator < other.m_iterator; }

private:
    Iter m_iterator;
};

template<Iterator Iter>
class MoveIterator {
public:
    using BaseValueType = Iter::ValueType;
    using ValueType = RemoveReference<BaseValueType>::type&&;

    constexpr explicit MoveIterator(Iter iter) : m_iterator(move(iter)) {}

    constexpr Iter base() const { return m_iterator; }

    constexpr ValueType operator*() const { return move(*m_iterator); }

    constexpr MoveIterator& operator++() {
        ++m_iterator;
        return *this;
    }

    constexpr MoveIterator operator++(int) const { return MoveIterator(m_iterator++); }

    constexpr bool operator==(const MoveIterator& other) const { return this->m_iterator == other.m_iterator; }
    constexpr bool operator!=(const MoveIterator& other) const { return this->m_iterator != other.m_iterator; }

private:
    Iter m_iterator;
};

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
        constexpr bool operator!=(const Iterator& other) const { return this->m_index != other.m_index; }
        constexpr bool operator<=(const Iterator& other) const { return this->m_index <= other.m_index; }
        constexpr bool operator<(const Iterator& other) const { return this->m_index > other.m_index; }
        constexpr bool operator>=(const Iterator& other) const { return this->m_index >= other.m_index; }
        constexpr bool operator>(const Iterator& other) const { return this->m_index > other.m_index; }

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
class Range {
public:
    explicit constexpr Range(T end) : m_start(), m_end(move(end)) {}
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
        constexpr bool operator!=(const Iterator& other) const { return this->m_value != other.m_value; }
        constexpr bool operator<=(const Iterator& other) const { return this->m_value <= other.m_value; }
        constexpr bool operator<(const Iterator& other) const { return this->m_value > other.m_value; }
        constexpr bool operator>=(const Iterator& other) const { return this->m_value >= other.m_value; }
        constexpr bool operator>(const Iterator& other) const { return this->m_value > other.m_value; }

    private:
        constexpr Iterator(T value) : m_value(move(value)) {}

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

template<Container C>
class Reversed {
public:
    explicit constexpr Reversed(C container) : m_container(::forward<C&&>(container)) {}

    constexpr auto begin() const {
        if constexpr (requires { m_container.rbegin(); }) {
            return m_container.rbegin();
        } else {
            return ReverseIterator(m_container.end());
        }
    }

    constexpr auto end() const {
        if constexpr (requires { m_container.rend(); }) {
            return m_container.rend();
        } else {
            return ReverseIterator(m_container.begin());
        }
    }

    constexpr auto size() const requires(SizedContainer<C>) { return m_container.size(); }

private : C m_container;
};

template<Container C>
class MoveElements {
public:
    explicit constexpr MoveElements(C&& container) : m_container(::move(container)) {}

    constexpr auto begin() { return MoveIterator(m_container.begin()); }
    constexpr auto end() { return MoveIterator(m_container.end()); }

    constexpr auto size() const requires(SizedContainer<C>) { return m_container.size(); }

private : C m_container;
};

template<Iterator Iter>
class IteratorContainer {
public:
    explicit constexpr IteratorContainer(Iter begin, Iter end) : m_begin(begin), m_end(end) {}

    constexpr Iter begin() const { return m_begin; }
    constexpr Iter end() const { return m_end; }

private:
    Iter m_begin;
    Iter m_end;
};

template<Iterator Iter>
class AutoSizedIteratorContainer {
public:
    explicit constexpr AutoSizedIteratorContainer(Iter begin, Iter end) : m_begin(begin), m_end(end) {}

    constexpr Iter begin() const { return m_begin; }
    constexpr Iter end() const { return m_end; }

    constexpr auto size() const { return m_end - m_begin; }

private:
    Iter m_begin;
    Iter m_end;
};

template<Iterator Iter>
class ExplicitlySizedIteratorContainer {
public:
    explicit constexpr ExplicitlySizedIteratorContainer(Iter begin, Iter end, size_t size) : m_begin(begin), m_end(end), m_size(size) {}

    constexpr Iter begin() const { return m_begin; }
    constexpr Iter end() const { return m_end; }

    constexpr size_t size() const { return m_size; }

private:
    Iter m_begin;
    Iter m_end;
    size_t m_size;
};

template<typename T>
constexpr Repeat<T> repeat(size_t count, T value) {
    return Repeat(count, move(value));
}

template<typename T>
constexpr Range<T> range(T start) {
    return Range(move(start));
}

template<typename T>
constexpr Range<T> range(T start, T end) {
    return Range(move(start), move(end));
}

template<Container T>
constexpr Reversed<T> reversed(T&& container) {
    return Reversed<T>(::forward<T>(container));
}

template<Container T>
constexpr MoveElements<T> move_elements(T&& container) {
    return MoveElements<T>(::forward<T>(container));
}

template<Iterator Iter>
constexpr auto iterator_container(Iter begin, Iter end) {
    if constexpr (requires { end - begin; }) {
        return AutoSizedIteratorContainer(begin, end);
    } else {
        return IteratorContainer(begin, end);
    }
}

template<Iterator Iter>
constexpr auto iterator_container(Iter begin, Iter end, size_t size) {
    if constexpr (requires { end - begin; }) {
        return AutoSizedIteratorContainer(begin, end);
    } else {
        return ExplicitlySizedIteratorContainer(begin, end, size);
    }
}
}

using LIIM::Container;
using LIIM::Iterator;
using LIIM::IteratorTraits;
using LIIM::MoveIterator;
using LIIM::Range;
using LIIM::range;
using LIIM::Repeat;
using LIIM::repeat;
using LIIM::ReverseIterator;
using LIIM::SizedContainer;
