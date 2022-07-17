#pragma once

#include <liim/container/concepts.h>
#include <liim/tuple.h>
#include <limits.h>

namespace LIIM::Container::View {
template<bool synchronized_end_iterators, Iterator... Iters>
class ZipIterator {
private:
    Tuple<Iters...> m_iterators;

    constexpr static bool is_copyable = conjunction<Copyable<Iters>...>;
    constexpr static bool is_double_ended = synchronized_end_iterators && conjunction<DoubleEndedIterator<Iters>...>;
    constexpr static bool is_random_access = synchronized_end_iterators && conjunction<RandomAccessIterator<Iters>...>;
    constexpr static bool is_mutable = conjunction<MutableIterator<Iters>...>;
    constexpr static bool is_comparable = synchronized_end_iterators && conjunction<Comparable<Iters>...>;

public:
    explicit constexpr ZipIterator(Tuple<Iters...> iterators) : m_iterators(move(iterators)) {}

    constexpr auto operator*() {
        return tuple_map(m_iterators, [](auto&& iterator) -> decltype(auto) {
            return (*iterator);
        });
    }

    using ValueType = decltype(*declval<ZipIterator>());

    constexpr decltype(auto) operator[](ssize_t index) const requires(is_random_access) { return *(*this + index); }

    constexpr ZipIterator& operator++() {
        tuple_visit(m_iterators, [](auto& iterator) {
            ++iterator;
        });
        return *this;
    }

    constexpr ZipIterator operator++(int) const requires(is_copyable) {
        auto result = *this;
        ++*this;
        return result;
    }

    constexpr ZipIterator& operator--() requires(is_double_ended) {
        tuple_visit(m_iterators, [](auto& iterator) {
            --iterator;
        });
        return *this;
    }

    constexpr ZipIterator operator--(int) requires(is_copyable&& is_double_ended) {
        auto result = *this;
        --*this;
        return result;
    }

    constexpr ZipIterator operator+(ssize_t n) const requires(is_random_access) {
        auto result = *this;
        result += n;
        return result;
    }

    constexpr ZipIterator operator-(ssize_t n) const requires(is_random_access) {
        auto result = *this;
        result -= n;
        return result;
    }

    constexpr ssize_t operator-(const ZipIterator& other) const requires(is_random_access) {
        return tuple_get<0>(this->m_iterators) - tuple_get<0>(other.m_iterators);
    }

    constexpr ZipIterator& operator+=(ssize_t n) requires(is_random_access) {
        tuple_visit(m_iterators, [&](auto& iterator) {
            iterator += n;
        });
        return *this;
    }

    constexpr ZipIterator& operator-=(ssize_t n) requires(is_random_access) {
        tuple_visit(m_iterators, [&](auto& iterator) {
            iterator -= n;
        });
        return *this;
    }

    constexpr bool operator==(const ZipIterator& other) const {
        auto helper = [&]<size_t... indices>(IndexSequence<indices...>)->bool {
            return ((tuple_get<indices>(this->m_iterators) == tuple_get<indices>(other.m_iterators)) || ...);
        };
        return helper(make_index_sequence<sizeof...(Iters)>());
    }

    constexpr auto operator<=>(const ZipIterator& other) const requires(is_comparable) = default;

    constexpr void swap_contents(ZipIterator other) requires(is_mutable) {
        auto helper = [&]<size_t... indices>(IndexSequence<indices...>) {
            ((swap_iterator_contents(tuple_get<indices>(this->m_iterators), tuple_get<indices>(other.m_iterators))), ...);
        };
        helper(make_index_sequence<sizeof...(Iters)>());
    }
};

template<Container... Cs>
class Zip {
    constexpr static bool is_synchronized = conjunction<RandomAccessContainer<Cs>...>;

public:
    explicit constexpr Zip(Cs&&... containers) : m_containers(forward<Cs>(containers)...) {}

    using Iterator = ZipIterator<is_synchronized, IteratorForContainer<Cs>...>;

    constexpr auto begin() {
        return Iterator(tuple_map(m_containers, [](auto&& container) {
            return container.begin();
        }));
    }
    constexpr auto end() {
        if constexpr (is_synchronized) {
            auto start = tuple_map(m_containers, [](auto&& container) {
                return container.begin();
            });
            auto end = tuple_map(m_containers, [](auto&& container) {
                return container.end();
            });
            auto get_sizes = [&]<size_t... indices>(IndexSequence<indices...>) {
                return Tuple((tuple_get<indices>(end) - tuple_get<indices>(start))...);
            };
            auto sizes = get_sizes(make_index_sequence<sizeof...(Cs)>());
            ssize_t min_size = SSIZE_MAX;
            tuple_visit(sizes, [&](auto size) {
                min_size = min(min_size, size);
            });
            tuple_visit(start, [&](auto& iterator) {
                iterator += min_size;
            });
            return Iterator(move(start));
        } else {
            return Iterator(tuple_map(m_containers, [](auto&& container) {
                return container.end();
            }));
        }
    }

private:
    Tuple<Cs...> m_containers;
};

template<Container... Cs>
requires(sizeof...(Cs) > 0) constexpr Zip<Cs...> zip(Cs&&... containers) {
    return Zip<Cs...>(forward<Cs>(containers)...);
}
}

using LIIM::Container::View::zip;
