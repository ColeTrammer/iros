#pragma once

#include <liim/container/erased_string.h>
#include <liim/error/domain.h>
#include <liim/error/transport.h>
#include <liim/tag_invoke.h>

namespace LIIM::Error {
inline constexpr struct IntoErasedError {
    template<typename T>
    constexpr auto operator()(T&& value) const -> TagInvokeResult<IntoErasedError, T&&> {
        return LIIM::tag_invoke(*this, forward<T>(value));
    }
} into_erased_error {};

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

    template<typename T>
    requires(SameAs<TagInvokeResult<Tag<into_erased_error>, T>, Error>) Error(T&& value) : Error(into_erased_error(forward<T>(value))) {}

    Error& operator=(Error&& other) {
        if (this != &other) {
            Error temp(move(other));
            swap(temp);
        }
        return *this;
    }

    ErasedString message() const {
        assert(m_domain);
        return m_domain->message(m_value);
    }
    ErasedString type() const {
        assert(m_domain);
        return m_domain->type();
    }

    void swap(Error& other) {
        ::swap(this->m_value, other.m_value);
        ::swap(this->m_domain, other.m_domain);
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

    constexpr Error& operator=(Error&& other) = default;

    constexpr ErasedString message() const { return m_domain->message(m_value); }
    constexpr ErasedString type() const { return m_domain->type(); }

    constexpr T& value() { return m_value; }
    constexpr const T& value() const { return m_value; }

    operator Error<>() requires(Erasable<T>) {
        ErrorTransport<T> transport { move(m_value) };
        return Error<>(ErrorTransport<>(transport.storage), m_domain);
    }

private:
    T m_value;
    const Domain* m_domain { nullptr };
};
}
