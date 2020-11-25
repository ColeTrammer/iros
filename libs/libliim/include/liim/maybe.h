#pragma once

#include <assert.h>
#include <liim/utilities.h>

namespace LIIM {

template<typename T>
class Maybe {
public:
    Maybe() {}
    Maybe(const T& value) : m_has_value(true) { new (&m_value[0]) T(value); }
    Maybe(T&& value) : m_has_value(true) { new (&m_value[0]) T(move(value)); }
    Maybe(const Maybe& other) : m_has_value(other.has_value()) {
        if (other.has_value()) {
            new (&m_value[0]) T(other.value());
        }
    }
    Maybe(Maybe&& other) : m_has_value(other.has_value()) {
        if (m_has_value) {
            new (&m_value[0]) T(LIIM::move(other.value()));
            other.value().~T();
            other.m_has_value = false;
        }
    }
    template<typename U>
    Maybe(const Maybe<U>& other) : m_has_value(other.has_value()) {
        if (other.has_value()) {
            new (&m_value[0]) T(other.value());
        }
    }
    template<typename U>
    Maybe(Maybe<U>&& other) : m_has_value(other.has_value()) {
        if (m_has_value) {
            new (&m_value[0]) T(LIIM::move(other.value()));
            other.value().~U();
            other.m_has_value = false;
        }
    }

    template<typename U>
    friend class Maybe;

    ~Maybe() {
        if (has_value()) {
            value().~T();
        }
    }

    Maybe<T>& operator=(const Maybe<T>& other) {
        if (this != &other) {
            Maybe<T> temp(other);
            swap(temp);
        }
        return *this;
    }

    Maybe<T>& operator=(Maybe<T>&& other) {
        if (this != &other) {
            Maybe<T> temp(LIIM::move(other));
            swap(temp);
        }
        return *this;
    }

    bool has_value() const { return m_has_value; }

    T& value() {
        assert(m_has_value);
        return *reinterpret_cast<T*>(&m_value[0]);
    }
    const T& value() const { return const_cast<Maybe<T>&>(*this).value(); }

    void swap(Maybe& other) {
        if (this->has_value() && other.has_value()) {
            LIIM::swap(this->value(), other.value());
        } else if (this->has_value()) {
            new (&other.m_value[0]) T(LIIM::move(this->value()));
            this->value().~T();
        } else if (other.has_value()) {
            new (&this->m_value[0]) T(LIIM::move(other.value()));
            other.value().~T();
        }
        LIIM::swap(this->m_has_value, other.m_has_value);
    }

private:
    char m_value[sizeof(T)];
    bool m_has_value { false };
};

template<typename T>
void swap(Maybe<T>& a, Maybe<T>& b) {
    a.swap(b);
}

}

using LIIM::Maybe;
