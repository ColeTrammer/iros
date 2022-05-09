#pragma once

#include <assert.h>
#include <liim/forward.h>
#include <liim/utilities.h>

namespace LIIM {
class None {};

template<typename T>
requires(!IsLValueReference<T>::value) class Maybe<T> {
public:
    constexpr Maybe() {}
    constexpr Maybe(None) {}
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
    constexpr bool operator==(const T& other) const { return this->has_value() && this->value() == other; }
    constexpr bool operator!=(const Maybe& other) const { return !(*this == other); }
    constexpr bool operator!=(const T& other) const { return !(*this == other); }

    constexpr T& operator*() { return value(); }
    constexpr const T& operator*() const { return value(); }

    constexpr T* operator->() { return &value(); }
    constexpr const T* operator->() const { return &value(); }

    constexpr T& value() {
        assert(m_has_value);
        return m_value;
    }
    constexpr const T& value() const { return const_cast<Maybe<T>&>(*this).value(); }

    constexpr T value_or(T default_value) const { return has_value() ? T { value() } : T { move(default_value) }; }

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

    template<typename C, typename R = InvokeResult<C>::type>
    constexpr Result<T, R> unwrap_or_else(C mapper) {
        if (has_value()) {
            return { move(value()) };
        }
        return { mapper() };
    }

    constexpr None error() const {
        assert(!has_value());
        return {};
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

template<typename T>
requires(IsLValueReference<T>::value) class Maybe<T> {
public:
    using ValueType = RemoveReference<T>::type;

    constexpr Maybe() {}
    constexpr Maybe(None) {}
    constexpr Maybe(T value) : m_value(&value) {}
    constexpr Maybe(ValueType* value) : m_value(value) {}
    constexpr Maybe(const Maybe& other) : m_value(other.m_value) {}
    constexpr Maybe(Maybe&& other) : m_value(exchange(other.m_value, nullptr)) {}

    template<typename U>
    requires(IsLValueReference<U>::value) constexpr Maybe(const Maybe<U>& other) : m_value(other.m_value) {}
    template<typename U>
    requires(IsLValueReference<U>::value) constexpr Maybe(Maybe<U>&& other) : m_value(exchange(other.m_value, nullptr)) {}

    template<typename U>
    friend class Maybe;

    constexpr ~Maybe() { reset(); }

    constexpr void reset() { m_value = nullptr; }

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

    constexpr bool has_value() const { return !!m_value; }
    constexpr bool operator!() const { return !has_value(); }
    constexpr operator bool() const { return has_value(); }

    constexpr bool operator==(const Maybe& other) const { return this->m_value == other.m_value; }
    constexpr bool operator==(const T& other) const { return this->m_value == &other; }
    constexpr bool operator!=(const Maybe& other) const { return !(*this == other); }
    constexpr bool operator!=(const T& other) const { return !(*this == other); }

    constexpr T& operator*() const { return value(); }
    constexpr ValueType* operator->() const { return &value(); }

    constexpr T value() const {
        assert(has_value());
        return *m_value;
    }

    constexpr ValueType& value_or(T default_value) const { return has_value() ? T { value() } : T { move(default_value) }; }

    template<typename... Args>
    constexpr ValueType& emplace(T& value) {
        m_value = &value;
    }

    constexpr void swap(Maybe& other) { LIIM::swap(this->m_value, other.m_value); }

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

    template<typename C, typename R = InvokeResult<C>::type>
    constexpr Result<T, R> unwrap_or_else(C mapper) {
        if (has_value()) {
            return { move(value()) };
        }
        return { mapper() };
    }

    constexpr None error() const {
        assert(!has_value());
        return {};
    }

private:
    ValueType* m_value { nullptr };
};
}

using LIIM::Maybe;
using LIIM::None;
