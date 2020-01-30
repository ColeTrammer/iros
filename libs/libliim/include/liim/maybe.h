#pragma once

#include <assert.h>
#include <liim/utilities.h>

namespace LIIM {

template<typename T> class Maybe {
public:
    Maybe() {}
    Maybe(const T& value) : m_has_value(true) { new (&m_value[0]) T(value); }
    Maybe(const Maybe<T>& other) : m_has_value(other.has_value()) {
        if (other.has_value()) {
            new (&m_value[0]) T(other.value());
        }
    }
    Maybe(Maybe<T>&& other) : m_has_value(other.has_value()) {
        if (m_has_value) {
            new (&m_value[0]) T(LIIM::move(*reinterpret_cast<T*>(&other.m_value[0])));
            other.m_value().~T();
            other.m_has_value = false;
        }
    }

    ~Maybe() {
        if (has_value()) {
            value().~T();
        }
    }

    Maybe<T>& operator=(const Maybe<T>& other) {
        if (!other.has_value()) {
            if (this->has_value()) {
                value().~T();
                m_has_value = false;
            }
        } else {
            if (this->has_value()) {
                value().~T();
            }

            new (&m_value[0]) T(other.value());
            m_has_value = true;
        }
        return *this;
    }

    Maybe<T>& operator=(Maybe&& other) {
        if (this != &other) {
            Maybe<T> temp(other);
            swap(other);
        }

        return *this;
    }

    bool has_value() const { return m_has_value; }

    T& value() {
        assert(m_has_value);
        return *reinterpret_cast<T*>(&m_value[0]);
    }
    const T& value() const { return const_cast<Maybe<T>&>(*this).value(); }

    void swap(Maybe<T>& other) {
        if (this->has_value() && other.has_value()) {
            LIIM::swap(this->value(), other.value());
        } else if (this->has_value()) {
            new (&other.m_value[0]) T(LIIM::move(*reinterpret_cast<T*>(&m_value[0])));
            this->value().~T();
        } else if (other.has_value()) {
            new (&this->m_value[0]) T(LIIM::move(*reinterpret_cast<T*>(&m_value[0])));
            other.value().~T();
        }
        LIIM::swap(this->m_has_value, other.m_has_value);
    }

private:
    char m_value[sizeof(T)];
    bool m_has_value { false };
};

template<typename T> void swap(Maybe<T>& a, Maybe<T>& b) {
    a.swap(b);
}

}

using LIIM::Maybe;