#pragma once

#include <assert.h>
#include <liim/forward.h>
#include <liim/utilities.h>

namespace LIIM {
class None {};

template<typename T>
requires(!IsLValueReference<T>::value) class Option<T> {
public:
    constexpr Option() {}
    constexpr Option(None) {}
    constexpr Option(const T& value) : m_value(value), m_has_value(true) {}
    constexpr Option(T&& value) : m_value(move(value)), m_has_value(true) {}
    constexpr Option(const Option& other) : m_has_value(other.has_value()) {
        if (other.has_value()) {
            new (&m_value) T { other.value() };
        }
    }
    constexpr Option(Option&& other) : m_has_value(other.has_value()) {
        if (m_has_value) {
            new (&m_value) T { LIIM::move(other.value()) };
            other.reset();
        }
    }
    template<typename U>
    constexpr Option(const Option<U>& other) : m_has_value(other.has_value()) {
        if (other.has_value()) {
            new (&m_value) T { other.value() };
        }
    }
    template<typename U>
    constexpr Option(Option<U>&& other) : m_has_value(other.has_value()) {
        if (m_has_value) {
            new (&m_value) T { LIIM::move(other.value()) };
            other.reset();
        }
    }

    template<typename U>
    friend class Option;

    constexpr ~Option() { reset(); }

    constexpr void reset() {
        if (has_value()) {
            value().~T();
            m_has_value = false;
        }
    }

    constexpr Option<T>& operator=(const Option<T>& other) {
        if (this != &other) {
            Option<T> temp { other };
            swap(temp);
        }
        return *this;
    }

    constexpr Option<T>& operator=(Option<T>&& other) {
        if (this != &other) {
            Option<T> temp { LIIM::move(other) };
            swap(temp);
        }
        return *this;
    }

    constexpr bool has_value() const { return m_has_value; }
    constexpr bool operator!() const { return !m_has_value; }
    constexpr operator bool() const { return m_has_value; }

    constexpr bool operator==(const Option& other) const {
        if (!this->has_value() && !other.has_value()) {
            return true;
        }
        return this->has_value() && other.has_value() && this->value() == other.value();
    }
    constexpr bool operator==(const T& other) const { return this->has_value() && this->value() == other; }
    constexpr bool operator!=(const Option& other) const { return !(*this == other); }
    constexpr bool operator!=(const T& other) const { return !(*this == other); }

    constexpr T& operator*() { return value(); }
    constexpr const T& operator*() const { return value(); }

    constexpr T* operator->() { return &value(); }
    constexpr const T* operator->() const { return &value(); }

    constexpr T& value() {
        assert(m_has_value);
        return m_value;
    }
    constexpr const T& value() const { return const_cast<Option<T>&>(*this).value(); }

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

    constexpr void swap(Option& other) {
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
    constexpr Option<R> map(C mapper) const {
        if (!has_value()) {
            return {};
        }
        return Option<R> { mapper(value()) };
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
constexpr void swap(Option<T>& a, Option<T>& b) {
    a.swap(b);
}

template<typename T>
requires(IsLValueReference<T>::value) class Option<T> {
public:
    using ValueType = RemoveReference<T>::type;

    constexpr Option() {}
    constexpr Option(None) {}
    constexpr Option(T value) : m_value(&value) {}
    constexpr Option(ValueType* value) : m_value(value) {}
    constexpr Option(const Option& other) : m_value(other.m_value) {}
    constexpr Option(Option&& other) : m_value(exchange(other.m_value, nullptr)) {}

    template<typename U>
    requires(IsLValueReference<U>::value) constexpr Option(const Option<U>& other) : m_value(other.m_value) {}
    template<typename U>
    requires(IsLValueReference<U>::value) constexpr Option(Option<U>&& other) : m_value(exchange(other.m_value, nullptr)) {}

    template<typename U>
    friend class Option;

    constexpr ~Option() { reset(); }

    constexpr void reset() { m_value = nullptr; }

    constexpr Option<T>& operator=(const Option<T>& other) {
        if (this != &other) {
            Option<T> temp { other };
            swap(temp);
        }
        return *this;
    }

    constexpr Option<T>& operator=(Option<T>&& other) {
        if (this != &other) {
            Option<T> temp { LIIM::move(other) };
            swap(temp);
        }
        return *this;
    }

    constexpr bool has_value() const { return !!m_value; }
    constexpr bool operator!() const { return !has_value(); }
    constexpr operator bool() const { return has_value(); }

    constexpr bool operator==(const Option& other) const { return this->m_value == other.m_value; }
    constexpr bool operator==(const T& other) const { return this->m_value == &other; }
    constexpr bool operator!=(const Option& other) const { return !(*this == other); }
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

    constexpr void swap(Option& other) { LIIM::swap(this->m_value, other.m_value); }

    template<typename C, typename R = InvokeResult<C, T>::type>
    constexpr Option<R> map(C mapper) const {
        if (!has_value()) {
            return {};
        }
        return Option<R> { mapper(value()) };
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

using LIIM::None;
using LIIM::Option;
