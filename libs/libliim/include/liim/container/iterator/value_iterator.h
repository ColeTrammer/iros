#pragma once

#include <assert.h>
#include <liim/container/container.h>

namespace LIIM::Container::Iterators {
template<typename T>
concept ValueIterator = requires(T iterator) {
    typename T::ValueType;
    { iterator.next() } -> SameAs<Option<typename T::ValueType>>;
};

template<typename Producer>
class ValueIteratorAdapterIterator {
public:
    explicit constexpr ValueIteratorAdapterIterator(Option<Producer&> producer) : m_producer(move(producer)) {
        if (m_producer) {
            ++(*this);
        }
    }

    constexpr ValueIteratorAdapterIterator(const ValueIteratorAdapterIterator&) = delete;
    constexpr ValueIteratorAdapterIterator(ValueIteratorAdapterIterator&&) = default;

    constexpr ValueIteratorAdapterIterator& operator=(const ValueIteratorAdapterIterator&) = delete;
    constexpr ValueIteratorAdapterIterator& operator=(ValueIteratorAdapterIterator&&) = default;

    using ValueType = Producer::ValueType;

    constexpr ValueType operator*() { return *move(m_cache); }

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

template<typename Self>
class ValueIteratorAdapter {
public:
    using Iterator = ValueIteratorAdapterIterator<Self>;
    using ConstIterator = Iterator;

    constexpr auto begin() { return Iterator(static_cast<Self&>(*this)); }
    constexpr auto end() const { return ConstIterator({}); }
};
}

using LIIM::Container::Iterators::ValueIteratorAdapter;
