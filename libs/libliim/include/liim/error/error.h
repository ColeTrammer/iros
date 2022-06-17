#pragma once

#include <liim/error/domain.h>
#include <liim/error/transport.h>
#include <liim/string_view.h>

namespace LIIM::Error {
template<>
class Error<void, ErrorDomain> {
public:
    constexpr Error(ErrorTransport<> value, const ErrorDomain* domain) : m_value(value), m_domain(domain) {}
    Error(Error&& other) : m_value(other.m_value), m_domain(exchange(other.m_domain, nullptr)) {}
    ~Error() {
        if (m_domain) {
            m_domain->destroy_error(m_value);
        }
    }

    StringView message() const {
        assert(m_domain);
        return m_domain->message(m_value);
    }
    StringView type() const {
        assert(m_domain);
        return m_domain->type();
    }

private:
    ErrorTransport<> m_value;
    const ErrorDomain* m_domain { nullptr };
};

template<typename T, typename Domain>
class Error {
public:
    constexpr Error(T&& value, const Domain* domain) : m_value(move(value)), m_domain(domain) {}
    constexpr Error(Error&& other) = default;
    constexpr ~Error() = default;

    constexpr StringView message() const { return m_domain->message(m_value); }
    constexpr StringView type() const { return m_domain->type(); }

    operator Error<>() requires(Erasable<T>) {
        ErrorTransport<T> transport { move(m_value) };
        return Error<>(ErrorTransport<>(transport.storage), m_domain);
    }

private:
    T m_value;
    const Domain* m_domain { nullptr };
};
}