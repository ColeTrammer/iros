#pragma once

#include <liim/initializer_list.h>
#include <liim/option.h>
#include <liim/tuple.h>
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
    { iterator == other } -> SameAs<bool>;
    { iterator != other } -> SameAs<bool>;
};

template<typename T>
concept ValueIterator = requires(T iterator) {
    typename T::ValueType;
    { iterator.next() } -> SameAs<Option<typename T::ValueType>>;
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

    constexpr ssize_t operator-(const ReverseIterator& other) const { return other.base() - this->base(); }

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
    using BaseValueType = IteratorTraits<Iter>::ValueType;
    static constexpr bool is_reference = IsReference<BaseValueType>::value;

    using ValueType = Conditional<is_reference, typename RemoveReference<BaseValueType>::type&&, BaseValueType>::type;

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

template<Iterator Iter>
class CountingIterator {
public:
    explicit constexpr CountingIterator(Iter iterator) : CountingIterator(move(iterator), 0) {}

    using BaseValueType = IteratorTraits<Iter>::ValueType;
    struct ValueType {
        size_t index;
        BaseValueType value;
    };

    constexpr Iter base() const { return m_iterator; }

    constexpr ValueType operator*() { return ValueType { m_index, *m_iterator }; }

    constexpr CountingIterator& operator++() {
        ++m_iterator;
        ++m_index;
        return *this;
    }

    constexpr CountingIterator operator++(int) const { return CountingIterator(m_iterator++, m_index + 1); }

    constexpr bool operator==(const CountingIterator& other) const { return this->m_iterator == other.m_iterator; }

private:
    constexpr CountingIterator(Iter iterator, size_t index) : m_iterator(move(iterator)), m_index(index) {}

    Iter m_iterator;
    size_t m_index { 0 };
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
class Enumerate {
public:
    explicit constexpr Enumerate(C container) : m_container(::forward<C&&>(container)) {}

    constexpr auto begin() { return CountingIterator(m_container.begin()); }
    constexpr auto end() { return CountingIterator(m_container.end()); }

    constexpr auto size() const requires(SizedContainer<C>) { return m_container.size(); }

private : C m_container;
};

template<Container C>
class Reversed {
public:
    explicit constexpr Reversed(C container) : m_container(::forward<C&&>(container)) {}

    constexpr auto begin() {
        if constexpr (requires { m_container.rbegin(); }) {
            return m_container.rbegin();
        } else {
            return ReverseIterator(m_container.end());
        }
    }

    constexpr auto end() {
        if constexpr (requires { m_container.rend(); }) {
            return m_container.rend();
        } else {
            return ReverseIterator(m_container.begin());
        }
    }

    constexpr auto size() const requires(SizedContainer<C>) { return m_container.size(); }

private : C m_container;
};

template<Iterator... Iters>
class ZipIterator {
private:
    Tuple<Iters...> m_iterators;

public:
    explicit constexpr ZipIterator(Tuple<Iters...> iterators) : m_iterators(move(iterators)) {}

    constexpr auto operator*() const {
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
    constexpr bool operator!=(const ZipIterator& other) const { return !(*this == other); }
};

template<Container... Cs>
class Zip {
public:
    explicit constexpr Zip(Cs&&... containers) : m_containers(::forward<Cs>(containers)...) {}

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
    explicit constexpr IteratorContainer(Iter begin, Iter end) : m_begin(move(begin)), m_end(move(end)) {}

    constexpr Iter begin() const { return m_begin; }
    constexpr Iter end() const { return m_end; }

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

template<typename Producer>
class ValueIteratorAdapterIterator {
public:
    explicit constexpr ValueIteratorAdapterIterator(Option<Producer&> producer) : m_producer(move(producer)) {
        if (m_producer) {
            ++(*this);
        }
    }

    using ValueType = Producer::ValueType;

    constexpr ValueType operator*() { return *m_cache; }

    constexpr ValueIteratorAdapterIterator& operator++() {
        assert(m_producer);
        m_cache = m_producer->next();
        return *this;
    }

    constexpr bool operator==(const ValueIteratorAdapterIterator& other) const {
        return (!this->m_producer || !this->m_cache) && (!other.m_producer || !other.m_cache);
    }

private:
    Option<Producer&> m_producer;
    Option<ValueType> m_cache;
};

template<typename Producer>
class ValueIteratorAdapter {
public:
    constexpr auto begin() { return ValueIteratorAdapterIterator<Producer>(static_cast<Producer&>(*this)); }
    constexpr auto end() { return ValueIteratorAdapterIterator<Producer>({}); }
};

template<typename T>
constexpr Repeat<T> repeat(size_t count, T value) {
    return Repeat(count, move(value));
}

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

template<Container T>
constexpr Reversed<T> reversed(T&& container) {
    return Reversed<T>(::forward<T>(container));
}

template<Container T>
constexpr Enumerate<T> enumerate(T&& container) {
    return Enumerate<T>(::forward<T>(container));
}

template<Container T>
constexpr MoveElements<T> move_elements(T&& container) {
    return MoveElements<T>(::forward<T>(container));
}

template<Container... Cs>
constexpr Zip<Cs...> zip(Cs&&... containers) {
    return Zip<Cs...>(::forward<Cs>(containers)...);
}

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

using LIIM::Container;
using LIIM::enumerate;
using LIIM::Iterator;
using LIIM::iterator_container;
using LIIM::IteratorTraits;
using LIIM::move_elements;
using LIIM::MoveIterator;
using LIIM::range;
using LIIM::repeat;
using LIIM::reversed;
using LIIM::ReverseIterator;
using LIIM::SizedContainer;
using LIIM::ValueIterator;
using LIIM::ValueIteratorAdapter;
