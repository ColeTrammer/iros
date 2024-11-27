#pragma once

#include <di/assert/assert_bool.h>
#include <di/container/string/erased_string.h>
#include <di/vocab/error/status_code_domain.h>

namespace di::vocab {
template<>
class StatusCode<void> {
public:
    using Domain = void;
    using Value = void;

    constexpr auto domain() const -> StatusCodeDomain const& {
        DI_ASSERT(!empty());
        return *m_domain;
    }

    [[nodiscard]] constexpr auto empty() const -> bool { return m_domain == nullptr; }

    constexpr auto message() const {
        if (!empty()) {
            return domain().do_message(*this);
        }
        return container::ErasedString(u8"[invalid status code]");
    }

    constexpr auto success() const -> bool { return !empty() && !domain().do_failure(*this); }
    constexpr auto failure() const -> bool { return !empty() && domain().do_failure(*this); }

    template<typename Domain>
    constexpr auto strictly_equivalent(StatusCode<Domain> const& other) const -> bool {
        if (this->empty() || other.empty()) {
            return this->empty() == other.empty();
        }
        return domain().do_equivalent(*this, other);
    }

    template<typename Domain>
    constexpr auto equivalent(StatusCode<Domain> const& other) const -> bool {
        return this->strictly_equivalent(other) || other.strictly_equivalent(*this);
    }

    constexpr auto generic_code() const -> GenericCode;

protected:
    constexpr explicit StatusCode(StatusCodeDomain const* domain) : m_domain(domain) {}

    StatusCode() = default;
    StatusCode(StatusCode const&) = default;
    StatusCode(StatusCode&&) = default;

    auto operator=(StatusCode const&) -> StatusCode& = default;
    auto operator=(StatusCode&&) -> StatusCode& = default;

    ~StatusCode() = default;

    StatusCodeDomain const* m_domain { nullptr };

private:
    template<typename Domain>
    friend class StatusCode;
};
}
