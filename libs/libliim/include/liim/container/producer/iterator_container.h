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

    template<size_t index>
    constexpr Iter get() {
        if constexpr (index == 0) {
            if constexpr (Copyable<Iter>) {
                return m_begin;
            } else {
                return move(m_begin);
            }
        } else if constexpr (index == 1) {
            if constexpr (Copyable<Iter>) {
                return m_end;
            } else {
                return move(m_end);
            }
        }
    }

private:
    Iter m_begin;
    Iter m_end;
};

template<Iterator Iter>
class AutoSizedIteratorContainer : public IteratorContainer<Iter> {
public:
    explicit constexpr AutoSizedIteratorContainer(Iter begin, Iter end) : IteratorContainer<Iter>(move(begin), move(end)) {}

    constexpr auto size() const { return this->end() - this->begin(); }
};

template<Iterator Iter>
class ExplicitlySizedIteratorContainer : public IteratorContainer<Iter> {
public:
    explicit constexpr ExplicitlySizedIteratorContainer(Iter begin, Iter end, size_t size)
        : IteratorContainer<Iter>(move(begin), move(end)), m_size(size) {}

    constexpr size_t size() const { return m_size; }

private:
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

namespace std {
template<LIIM::Container::Iterator Iter>
struct tuple_size<LIIM::Container::Producer::IteratorContainer<Iter>> {
    constexpr static size_t value = 2;
};

template<size_t index, LIIM::Container::Iterator Iter>
struct tuple_element<index, LIIM::Container::Producer::IteratorContainer<Iter>> {
    using type = Iter;
};

template<LIIM::Container::Iterator Iter>
struct tuple_size<LIIM::Container::Producer::AutoSizedIteratorContainer<Iter>> {
    constexpr static size_t value = 2;
};

template<size_t index, LIIM::Container::Iterator Iter>
struct tuple_element<index, LIIM::Container::Producer::AutoSizedIteratorContainer<Iter>> {
    using type = Iter;
};

template<LIIM::Container::Iterator Iter>
struct tuple_size<LIIM::Container::Producer::ExplicitlySizedIteratorContainer<Iter>> {
    constexpr static size_t value = 2;
};

template<size_t index, LIIM::Container::Iterator Iter>
struct tuple_element<index, LIIM::Container::Producer::ExplicitlySizedIteratorContainer<Iter>> {
    using type = Iter;
};
}

using LIIM::Container::Producer::iterator_container;
