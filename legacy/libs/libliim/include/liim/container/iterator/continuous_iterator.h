#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::Iterators {
template<typename Self>
class ContinuousIteratorAdapter {
public:
    constexpr decltype(auto) operator[](ssize_t index) const { return *(*this + index); }

    constexpr Self& operator++() {
        ++m_index;
        return static_cast<Self&>(*this);
    }
    constexpr Self& operator--() {
        --m_index;
        return static_cast<Self&>(*this);
    }

    constexpr Self operator++(int) {
        auto result = Self(static_cast<const Self&>(*this));
        ++*this;
        return result;
    }
    constexpr Self operator--(int) {
        auto result = Self(static_cast<const Self&>(*this));
        --*this;
        return result;
    }

    constexpr Self operator+(ssize_t n) const {
        auto result = Self(static_cast<const Self&>(*this));
        result.m_index += n;
        return result;
    }

    constexpr Self operator-(ssize_t n) const {
        auto result = Self(static_cast<const Self&>(*this));
        result.m_index -= n;
        return result;
    }

    constexpr ssize_t operator-(const Self& other) const { return this->index() - other.index(); }

    constexpr Self& operator+=(ssize_t n) {
        m_index += n;
        return static_cast<Self&>(*this);
    }
    constexpr Self& operator-=(ssize_t n) {
        m_index -= n;
        return static_cast<Self&>(*this);
    }

    constexpr friend bool operator==(const Self& a, const Self& b) { return a.index() == b.index(); }
    constexpr friend auto operator<=>(const Self& a, const Self& b) { return a.index() <=> b.index(); }

protected:
    constexpr explicit ContinuousIteratorAdapter(size_t index) : m_index(index) {}

    constexpr size_t index() const { return m_index; }

private:
    size_t m_index { 0 };
};
}

using LIIM::Container::Iterators::ContinuousIteratorAdapter;
