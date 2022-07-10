#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::View {
template<Iterator Iter, typename F>
class TransformIterator {
public:
    explicit constexpr TransformIterator(Iter iterator, F& transformer) : m_iterator(move(iterator)), m_transformer(&transformer) {}

    using ValueType = InvokeResult<F, typename IteratorTraits<Iter>::ValueType>::type;

    constexpr ValueType operator*() { return (*m_transformer)(*m_iterator); }

    constexpr TransformIterator& operator++() {
        ++m_iterator;
        return *this;
    }

    constexpr TransformIterator operator++(int) {
        auto result = TransformIterator(*this);
        ++*this;
        return result;
    }

    constexpr bool operator==(const TransformIterator& other) const { return this->m_iterator == other.m_iterator; }

private:
    Iter m_iterator;
    F* m_transformer;
};

template<Container C, typename F>
class Transform {
public:
    explicit constexpr Transform(C&& container, F&& transformer)
        : m_container(forward<C>(container)), m_transformer(forward<F>(transformer)) {}

    constexpr auto begin() { return TransformIterator(m_container.begin(), m_transformer); }
    constexpr auto end() { return TransformIterator(m_container.end(), m_transformer); }

    constexpr auto size() const requires(SizedContainer<C>) { return m_container.size(); }

private:
    C m_container;
    F m_transformer;
};

template<Container T, typename F>
constexpr Transform<T, F> transform(T&& container, F&& transformer) {
    return Transform<T, F>(forward<T>(container), forward<F>(transformer));
}
}

using LIIM::Container::View::transform;
