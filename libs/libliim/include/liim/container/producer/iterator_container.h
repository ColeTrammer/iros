#pragma once

#include <liim/construct.h>
#include <liim/container/concepts.h>

namespace LIIM::Container::Producer {
template<Iterator Iter>
class IteratorContainer {
public:
    explicit constexpr IteratorContainer(Iter begin, Iter end) : m_begin(move(begin)), m_end(move(end)) {}

    constexpr Iter begin() const requires(Copyable<Iter>) { return m_begin; }
    constexpr Iter end() const requires(Copyable<Iter>) { return m_end; }

    constexpr Iter begin() requires(!Copyable<Iter>) { return move(m_begin); }
    constexpr Iter end() requires(!Copyable<Iter>) { return move(m_end); }

private:
    Iter m_begin;
    Iter m_end;
};

template<Iterator Iter>
class AutoSizedIteratorContainer {
public:
    explicit constexpr AutoSizedIteratorContainer(Iter begin, Iter end) : m_begin(move(begin)), m_end(move(end)) {}

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
    explicit constexpr ExplicitlySizedIteratorContainer(Iter begin, Iter end, size_t size)
        : m_begin(move(begin)), m_end(move(end)), m_size(size) {}

    constexpr Iter begin() const { return m_begin; }
    constexpr Iter end() const { return m_end; }

    constexpr size_t size() const { return m_size; }

private:
    Iter m_begin;
    Iter m_end;
    size_t m_size;
};

template<Iterator Iter>
constexpr auto iterator_container(Iter begin, Iter end) {
    if constexpr (requires { end - begin; }) {
        return AutoSizedIteratorContainer(move(begin), move(end));
    } else {
        return IteratorContainer(move(begin), move(end));
    }
}

template<Iterator Iter>
constexpr auto iterator_container(Iter begin, Iter end, size_t size) {
    if constexpr (requires { end - begin; }) {
        return AutoSizedIteratorContainer(move(begin), move(end));
    } else {
        return ExplicitlySizedIteratorContainer(move(begin), move(end), size);
    }
}
}

using LIIM::Container::Producer::iterator_container;
