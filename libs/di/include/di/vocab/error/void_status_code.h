#pragma once

#include <di/assert/prelude.h>
#include <di/container/string/erased_string.h>
#include <di/vocab/error/status_code_domain.h>

namespace di::vocab {
template<>
class StatusCode<void> {
public:
    using Domain = void;
    using Value = void;

    constexpr StatusCodeDomain const& domain() const {
        DI_ASSERT(!empty());
        return *m_domain;
    }

    [[nodiscard]] constexpr bool empty() const { return m_domain == nullptr; }

    constexpr auto message() const {
        if (!empty()) {
            return domain().do_message(*this);
        }
        return container::ErasedString(u8"[invalid status code]");
    }

    constexpr bool success() const { return !empty() && !domain().do_failure(*this); }
    constexpr bool failure() const { return !empty() && domain().do_failure(*this); }

    template<typename Domain>
    constexpr bool strictly_equivalent(StatusCode<Domain> const& other) const {
        if (this->empty() || other.empty()) {
            return this->empty() == other.empty();
        }
        return domain().do_equivalent(*this, other);
    }

    template<typename Domain>
    constexpr bool equivalent(StatusCode<Domain> const& other) const {
        return this->strictly_equivalent(other) || other.strictly_equivalent(*this);
    }

protected:
    constexpr explicit StatusCode(StatusCodeDomain const* domain) : m_domain(domain) {}

    StatusCode() = default;
    StatusCode(StatusCode const&) = default;
    StatusCode(StatusCode&&) = default;

    StatusCode& operator=(StatusCode const&) = default;
    StatusCode& operator=(StatusCode&&) = default;

    ~StatusCode() = default;

    StatusCodeDomain const* m_domain { nullptr };

private:
    template<typename Domain>
    friend class StatusCode;
};
}
