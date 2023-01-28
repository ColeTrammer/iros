#pragma once

#include <di/util/address_of.h>
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

    constexpr Domain const& domain() const {
        DI_ASSERT(!empty());
        return static_cast<Domain const&>(*m_domain);
    }

    constexpr void clear() {
        util::destroy_at(util::address_of(m_value));
        m_domain = nullptr;
        util::construct_at(util::address_of(m_value));
    }

    constexpr Value& value() & { return m_value; }
    constexpr Value const& value() const& { return m_value; }
    constexpr Value&& value() && { return util::move(m_value); }
    constexpr Value const&& value() const&& { return util::move(m_value); }

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

    StatusCodeStorage& operator=(StatusCodeStorage const&) = default;
    constexpr StatusCodeStorage& operator=(StatusCodeStorage&& other) {
        util::destroy_at(util::address_of(m_value));
        util::construct_at(util::address_of(m_value), util::move(other).value());
        return *this;
    }

    ~StatusCodeStorage() = default;

    Value m_value {};
};
}