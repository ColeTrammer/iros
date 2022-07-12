#pragma once

#include <liim/container/concepts.h>
#include <liim/container/iterator/swap_iterator_contents.h>

namespace LIIM::Container::Iterators {
template<typename Self, Iterator Iter>
class WrapperIteratorAdapter {
public:
    constexpr Iter& base() { return m_base; }
    constexpr const Iter& base() const { return m_base; }

    constexpr decltype(auto) operator[](ssize_t index) requires(RandomAccessIterator<Iter>) { return *(*this + index); }

    constexpr Self& operator++() {
        ++m_base;
        return static_cast<Self&>(*this);
    }

    constexpr Self operator++(int) requires(Copyable<Self>) {
        auto result = Self(static_cast<const Self&>(*this));
        ++*this;
        return result;
    }

    constexpr Self& operator--() requires(DoubleEndedIterator<Iter>) {
        --m_base;
        return static_cast<Self&>(*this);
    }

    constexpr Self operator--(int) requires(DoubleEndedIterator<Iter>&& Copyable<Self>) {
        auto result = Self(static_cast<const Self&>(*this));
        --*this;
        return result;
    }

    constexpr Self operator+(ssize_t n) const requires(RandomAccessIterator<Iter>) {
        auto result = Self(static_cast<const Self&>(*this));
        result += n;
        return result;
    }

    constexpr Self operator-(ssize_t n) const requires(RandomAccessIterator<Iter>) {
        auto result = Self(static_cast<const Self&>(*this));
        result -= n;
        return result;
    }

    constexpr ssize_t operator-(const Self& other) const requires(RandomAccessIterator<Iter>) { return this->base() - other.base(); }

    constexpr Self& operator+=(ssize_t n) requires(RandomAccessIterator<Iter>) {
        m_base += n;
        return static_cast<Self&>(*this);
    }

    constexpr Self& operator-=(ssize_t n) requires(RandomAccessIterator<Iter>) {
        m_base -= n;
        return static_cast<Self&>(*this);
    }

    constexpr friend bool operator==(const Self& a, const Self& b) { return a.base() == b.base(); }

    constexpr friend auto operator<=>(const Self& a, const Self& b) requires(Comparable<Iter>) { return a.base() <=> b.base(); }

    constexpr void swap_contents(Self other) requires(MutableIterator<Iter>) { swap_iterator_contents(this->base(), other.base()); }

protected:
    constexpr explicit WrapperIteratorAdapter(Iter base) : m_base(move(base)) {}

private:
    Iter m_base;
};
}

using LIIM::Container::Iterators::WrapperIteratorAdapter;
