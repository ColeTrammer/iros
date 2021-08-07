#pragma once

#include <assert.h>
#include <liim/utilities.h>

namespace LIIM {

template<typename T>
class Maybe {
public:
    constexpr Maybe() {}
    constexpr Maybe(const T& value) : m_value(value), m_has_value(true) {}
    constexpr Maybe(T&& value) : m_value(move(value)), m_has_value(true) {}
    constexpr Maybe(const Maybe& other) : m_has_value(other.has_value()) {
        if (other.has_value()) {
            new (&m_value) T { other.value() };
        }
    }
    constexpr Maybe(Maybe&& other) : m_has_value(other.has_value()) {
        if (m_has_value) {
            new (&m_value) T { LIIM::move(other.value()) };
            other.reset();
        }
    }
    template<typename U>
    constexpr Maybe(const Maybe<U>& other) : m_has_value(other.has_value()) {
        if (other.has_value()) {
            new (&m_value) T { other.value() };
        }
    }
    template<typename U>
    constexpr Maybe(Maybe<U>&& other) : m_has_value(other.has_value()) {
        if (m_has_value) {
            new (&m_value) T { LIIM::move(other.value()) };
            other.reset();
        }
    }

    template<typename U>
    friend class Maybe;

    constexpr ~Maybe() { reset(); }

    constexpr void reset() {
        if (has_value()) {
            value().~T();
            m_has_value = false;
        }
    }

    constexpr Maybe<T>& operator=(const Maybe<T>& other) {
        if (this != &other) {
            Maybe<T> temp { other };
            swap(temp);
        }
        return *this;
    }

    constexpr Maybe<T>& operator=(Maybe<T>&& other) {
        if (this != &other) {
            Maybe<T> temp { LIIM::move(other) };
            swap(temp);
        }
        return *this;
    }

    constexpr bool has_value() const { return m_has_value; }
    constexpr bool operator!() const { return !m_has_value; }
    constexpr operator bool() const { return m_has_value; }

    constexpr bool operator==(const Maybe& other) const {
        if (!this->has_value() && !other.has_value()) {
            return true;
        }
        return this->has_value() && other.has_value() && this->value() == other.value();
    }
    constexpr bool operator!=(const Maybe& other) const { return !(*this == other); }

    constexpr T& operator*() { return value(); }
    constexpr const T& operator*() const { return value(); }

    constexpr T* operator->() { return &value(); }
    constexpr const T* operator->() const { return &value(); }

    constexpr T& value() {
        assert(m_has_value);
        return m_value;
    }
    constexpr const T& value() const { return const_cast<Maybe<T>&>(*this).value(); }

    constexpr T value_or(T&& default_value) const { return has_value() ? value() : default_value; }

    template<typename... Args>
    constexpr T& emplace(Args&&... args) {
        if (has_value()) {
            reset();
        }
        new (&m_value) T { forward<Args>(args)... };
        m_has_value = true;
        return value();
    }

    constexpr void swap(Maybe& other) {
        if (this->has_value() && other.has_value()) {
            LIIM::swap(this->value(), other.value());
        } else if (this->has_value()) {
            other.emplace(LIIM::move(this->value()));
            this->reset();
        } else if (other.has_value()) {
            this->emplace(LIIM::move(other.value()));
            other.reset();
        }
    }

    template<typename C, typename R = InvokeResult<C, T>::type>
    constexpr Maybe<R> map(C mapper) const {
        if (!has_value()) {
            return {};
        }
        return Maybe<R> { mapper(value()) };
    }

    template<typename C, typename R = InvokeResult<C, T>::type>
    constexpr R and_then(C mapper) const {
        if (!has_value()) {
            return {};
        }
        return mapper(value());
    }

private:
    union {
        char m_empty;
        T m_value;
    };
    bool m_has_value { false };
};

template<typename T>
constexpr void swap(Maybe<T>& a, Maybe<T>& b) {
    a.swap(b);
}

}

using LIIM::Maybe;
