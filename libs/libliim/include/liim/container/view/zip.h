#pragma once

#include <liim/container/concepts.h>
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

template<Container... Cs>
constexpr Zip<Cs...> zip(Cs&&... containers) {
    return Zip<Cs...>(forward<Cs>(containers)...);
}
}

using LIIM::Container::View::zip;
