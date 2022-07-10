#pragma once

#include <liim/container/algorithm/equal.h>
#include <liim/container/algorithm/lexographic_compare.h>
#include <liim/container/iterator/reverse_iterator.h>
#include <liim/container/view/zip.h>
#include <liim/option.h>
#include <liim/span.h>
#include <liim/utilities.h>

namespace LIIM::Container {
template<typename T, size_t N>
struct Array {
    using ValueType = T;
    using Iterator = T*;
    using ConstIterator = const T*;
    using ReverseIterator = ::ReverseIterator<Iterator>;
    using ReverseConstIterator = ::ReverseIterator<ConstIterator>;

    constexpr Option<T&> at(size_t index) {
        if (index >= N) {
            return None {};
        }
        return m_data[index];
    }

    constexpr Option<const T&> at(size_t index) const {
        if (index >= N) {
            return None {};
        }
        return m_data[index];
    }

    constexpr decltype(auto) operator[](size_t index) {
        assert(index < N);
        return m_data[index];
    }

    constexpr decltype(auto) operator[](size_t index) const {
        assert(index < N);
        return m_data[index];
    }

    constexpr decltype(auto) front() { return (*this)[0]; }
    constexpr decltype(auto) front() const { return (*this)[0]; }

    constexpr decltype(auto) back() { return (*this)[N - 1]; }
    constexpr decltype(auto) back() const { return (*this)[N - 1]; }

    constexpr T* data() { return m_data; }
    constexpr const T* data() const { return m_data; }

    constexpr Span<T> span() { return { data(), N }; }
    constexpr Span<const T> span() const { return { data(), N }; }

    constexpr Iterator begin() { return data(); }
    constexpr Iterator end() { return data() + N; }
    constexpr ConstIterator begin() const { return data(); }
    constexpr ConstIterator end() const { return data() + N; }
    constexpr ConstIterator cbegin() const { return begin(); }
    constexpr ConstIterator cend() const { return end(); }

    constexpr ReverseIterator rbegin() { return ReverseIterator(end()); }
    constexpr ReverseIterator rend() { return ReverseIterator(begin()); }
    constexpr ReverseConstIterator rbegin() const { return ReverseConstIterator(end()); }
    constexpr ReverseConstIterator rend() const { return ReverseConstIterator(begin()); }
    constexpr ReverseConstIterator crbegin() const { return rbegin(); }
    constexpr ReverseConstIterator crend() const { return rend(); }

    constexpr bool empty() const { return N == 0; }
    constexpr size_t size() const { return N; }
    constexpr size_t max_size() const { return N; }

    constexpr void fill(const T& value) {
        for (auto& x : *this) {
            x = value;
        }
    }

    constexpr void swap(Array& other) {
        for (auto [a, b] : zip(*this, other)) {
            ::swap(a, b);
        }
    }

    constexpr bool operator==(const Array& other) const requires(EqualComparable<T>) { return equal(*this, other); }
    constexpr auto operator<=>(const Array& other) const requires(Comparable<T>) { return lexographic_compare(*this, other); }

    template<size_t index>
    constexpr decltype(auto) get() & {
        static_assert(index < N);
        return (*this)[index];
    }

    template<size_t index>
    constexpr decltype(auto) get() const& {
        static_assert(index < N);
        return (*this)[index];
    }

    template<size_t index>
    constexpr decltype(auto) get() && {
        static_assert(index < N);
        return move((*this)[index]);
    }

    template<size_t index>
    constexpr decltype(auto) get() const&& {
        static_assert(index < N);
        return move((*this)[index]);
    }

    T m_data[N];
};

template<class T, class... U>
Array(T, U...) -> Array<T, 1 + sizeof...(U)>;

template<typename T, size_t N>
constexpr auto make_array(T (&input)[N]) {
    auto factory = [&]<size_t... indices>(IndexSequence<indices...>)->Array<typename RemoveCV<T>::type, N> {
        return { { input[indices]... } };
    };
    return factory(make_index_sequence<N>());
}

template<typename T, size_t N>
constexpr auto make_array(T (&&input)[N]) {
    auto factory = [&]<size_t... indices>(IndexSequence<indices...>)->Array<typename RemoveCV<T>::type, N> {
        return { { move(input[indices])... } };
    };
    return factory(make_index_sequence<N>());
}
}

namespace std {
template<typename T, size_t N>
struct tuple_size<LIIM::Container::Array<T, N>> {
    constexpr static size_t value = N;
};

template<size_t I, typename T, size_t N>
struct tuple_element<I, LIIM::Container::Array<T, N>> {
    using type = T;
};
}

using LIIM::Container::Array;
using LIIM::Container::make_array;
