#pragma once

#include <liim/container/concepts.h>
#include <liim/pair.h>
#include <liim/tuple.h>

namespace LIIM::Container::View {
template<Iterator... Iters>
class ZipIterator {
private:
    Tuple<Iters...> m_iterators;

public:
    explicit constexpr ZipIterator(Tuple<Iters...> iterators) : m_iterators(move(iterators)) {}

    constexpr auto operator*() {
        return tuple_map(m_iterators, [](auto&& iterator) -> decltype(auto) {
            return (*iterator);
        });
    }

    using ValueType = decltype(*declval<ZipIterator>());

    constexpr ZipIterator& operator++() {
        tuple_map(m_iterators, [](auto& iterator) {
            ++iterator;
            return 0;
        });
        return *this;
    }

    constexpr ZipIterator operator++(int) const {
        auto result = *this;
        ++*this;
        return result;
    }

    constexpr bool operator==(const ZipIterator& other) const {
        auto helper = [&]<size_t... indices>(IndexSequence<indices...>)->bool {
            return ((this->m_iterators.template get<indices>() == other.m_iterators.template get<indices>()) || ...);
        };
        return helper(make_index_sequence<sizeof...(Iters)>());
    }
};

template<Iterator I, Iterator J>
class ZipIterator<I, J> {
private:
    I m_a;
    J m_b;

public:
    explicit constexpr ZipIterator(I a, J b) : m_a(move(a)), m_b(move(b)) {}

    using ValueType = Pair<typename IteratorTraits<I>::ValueType, typename IteratorTraits<J>::ValueType>;

    constexpr ValueType operator*() { return ValueType(*m_a, *m_b); }

    constexpr ZipIterator& operator++() {
        ++m_a;
        ++m_b;
        return *this;
    }

    constexpr ZipIterator operator++(int) const {
        auto result = *this;
        ++*this;
        return result;
    }

    constexpr bool operator==(const ZipIterator& other) const { return this->m_a == other.m_a || this->m_b == other.m_b; }
};

template<Container... Cs>
class Zip {
public:
    explicit constexpr Zip(Cs&&... containers) : m_containers(forward<Cs>(containers)...) {}

    constexpr auto begin() {
        return ZipIterator(tuple_map(m_containers, [](auto&& container) {
            return container.begin();
        }));
    }
    constexpr auto end() {
        return ZipIterator(tuple_map(m_containers, [](auto&& container) {
            return container.end();
        }));
    }

private:
    Tuple<Cs...> m_containers;
};

template<Container C, Container D>
class Zip<C, D> {
public:
    explicit constexpr Zip(C&& c, D&& d) : m_c(forward<C>(c)), m_d(forward<D>(d)) {}

    using Iterator = ZipIterator<decltype(declval<C>().begin()), decltype(declval<D>().begin())>;

    constexpr auto begin() { return Iterator(m_c.begin(), m_d.begin()); }
    constexpr auto end() { return Iterator(m_c.end(), m_d.end()); }

private:
    C m_c;
    D m_d;
};

template<Container... Cs>
constexpr Zip<Cs...> zip(Cs&&... containers) {
    return Zip<Cs...>(forward<Cs>(containers)...);
}
}

using LIIM::Container::View::zip;
