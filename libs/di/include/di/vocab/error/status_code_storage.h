#pragma once

#include <di/util/addressof.h>
#include <di/util/destroy_at.h>
#include <di/vocab/error/erased.h>
#include <di/vocab/error/meta/status_code_domain.h>
#include <di/vocab/error/meta/status_code_domain_value.h>
#include <di/vocab/error/void_status_code.h>

namespace di::vocab::detail {
template<typename T>
requires(!concepts::LanguageVoid<T>)
class StatusCodeStorage : public StatusCode<void> {
private:
    using Base = StatusCode<void>;

public:
    using Domain = meta::StatusCodeDomain<T>;
    using Value = meta::StatusCodeDomainValue<T>;

    constexpr auto domain() const -> Domain const& {
        DI_ASSERT(!empty());
        return static_cast<Domain const&>(*m_domain);
    }

    constexpr void clear() {
        util::destroy_at(util::addressof(m_value));
        m_domain = nullptr;
        util::construct_at(util::addressof(m_value));
    }

    constexpr auto value() & -> Value& { return m_value; }
    constexpr auto value() const& -> Value const& { return m_value; }
    constexpr auto value() && -> Value&& { return util::move(m_value); }
    constexpr auto value() const&& -> Value const&& { return util::move(m_value); }

protected:
    StatusCodeStorage() = default;
    StatusCodeStorage(StatusCodeStorage const&) = default;
    constexpr StatusCodeStorage(StatusCodeStorage&& other)
        : Base(util::move(other)), m_value(util::move(other).value()) {
        other.m_domain = nullptr;
    }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr StatusCodeStorage(InPlace, StatusCodeDomain const* domain, Args&&... args)
        : Base(domain), m_value(util::forward<Args>(args)...) {}

    auto operator=(StatusCodeStorage const&) -> StatusCodeStorage& = default;
    constexpr auto operator=(StatusCodeStorage&& other) -> StatusCodeStorage& {
        util::destroy_at(util::addressof(m_value));
        util::construct_at(util::addressof(m_value), util::move(other).value());
        return *this;
    }

    ~StatusCodeStorage() = default;

    Value m_value {};
};
}
