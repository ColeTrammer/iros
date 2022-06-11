#pragma once

#include <liim/forward.h>
#include <liim/string_view.h>

namespace LIIM {
class ErrorDomain;

template<typename T = void, typename Domain = ErrorDomain>
class Error;

template<typename T = void>
struct ErrorTransport;

template<>
struct ErrorTransport<void> {
    void* value;
};

template<typename T>
requires(sizeof(T) <= sizeof(void*) && alignof(T) <= sizeof(void*)) struct ErrorTransport<T> {
    union {
        T value;
        void* storage;
    };
};

class ErrorDomain {
public:
    constexpr ErrorDomain() = default;
    constexpr virtual ~ErrorDomain() = default;

    virtual void destroy_error(ErrorTransport<>& value) = 0;
    virtual StringView message(const ErrorTransport<>& value) const = 0;
    virtual StringView type() const = 0;
};

template<>
class Error<void, ErrorDomain> {
public:
    Error(Error&& other) : m_value(other.m_value), m_domain(exchange(other.m_domain, nullptr)) {}
    ~Error() {
        if (m_domain) {
            m_domain->destroy_error(m_value);
        }
    }

    StringView message() const { return m_domain->message(m_value); }
    StringView type() const { return m_domain->type(); }

private:
    ErrorTransport<> m_value;
    ErrorDomain* m_domain { nullptr };
};

template<typename T, typename Domain>
class Error {
public:
    constexpr Error(Error&& other) = default;
    constexpr ~Error() = default;

    constexpr StringView message() const { return m_domain->message(m_value); }
    constexpr StringView type() const { return m_domain->type(); }

private:
    T m_value;
    Domain* m_domain { nullptr };
};
}
using LIIM::Error;
