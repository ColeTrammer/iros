#pragma once

#include <di/vocab/error/concepts/erased_status_code.h>
#include <di/vocab/error/concepts/status_code_erasable_into.h>
#include <di/vocab/error/erased.h>
#include <di/vocab/error/erasure_cast.h>
#include <di/vocab/error/into_status_code.h>
#include <di/vocab/error/status_code_storage.h>

namespace di::vocab {
template<typename T>
class StatusCode<Erased<T>> : public detail::StatusCodeStorage<Erased<T>> {
private:
    using Base = detail::StatusCodeStorage<Erased<T>>;

public:
    using Domain = void;
    using Value = T;

    StatusCode() = default;
    StatusCode(StatusCode const&) = delete;
    StatusCode(StatusCode&&) = default;

    template<typename Domain>
    requires(concepts::StatusCodeErasableInto<Domain, Erased<T>> && !concepts::ErasedStatusCode<StatusCode<meta::Decay<Domain>>>)
    constexpr StatusCode(StatusCode<Domain> const& other) : Base(in_place, other.m_domain, detail::erasure_cast<Value>(other.value())) {}

    template<typename Domain>
    requires(concepts::StatusCodeErasableInto<Domain, Erased<T>> && !concepts::ErasedStatusCode<StatusCode<meta::Decay<Domain>>>)
    constexpr StatusCode(StatusCode<Domain>&& other) : Base(in_place, other.m_domain, detail::erasure_cast<Value>(other.value())) {}

    template<typename U, typename... Args>
    requires(!concepts::DecaySameAs<U, StatusCode> && !concepts::DecaySameAs<U, Value> &&
             concepts::ConvertibleToStatusCode<StatusCode, U, Args...>)
    constexpr StatusCode(U&& v, Args&&... args) : StatusCode(into_status_code(util::forward<U>(v), util::forward<Args>(args)...)) {}

    StatusCode& operator=(StatusCode const&) = delete;
    StatusCode& operator=(StatusCode&&) = default;

    constexpr ~StatusCode() {
        if (this->m_domain) {
            this->domain().do_erased_destroy(*this, sizeof(*this));
        }
    }

    constexpr auto value() const { return this->m_value; }

private:
    template<typename Domain>
    friend class StatusCode;
};
}