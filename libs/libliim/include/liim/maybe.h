#pragma once

#include <assert.h>
#include <liim/utilities.h>

template<typename T> class Maybe {
public:
    Maybe() {}
    Maybe(const T& value) : m_has_value(true) { new (&m_value[0]) T(value); }
    Maybe(const Maybe<T>& other) : m_has_value(other.has_value()) {
        if (other.has_value()) {
            new (&m_value[0]) T(other.value());
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

    bool has_value() const { return m_has_value; }

    T& value() {
        assert(m_has_value);
        return *reinterpret_cast<T*>(&m_value[0]);
    }
    const T& value() const { return const_cast<Maybe<T>&>(*this).value(); }

private:
    char m_value[sizeof(T)];
    bool m_has_value { false };
};