#pragma once

#include <di/vocab/error/concepts/status_code_erasable_into.h>
#include <di/vocab/error/into_status_code.h>
#include <di/vocab/error/status_code_domain.h>

namespace di::vocab {
template<concepts::DerivedFrom<StatusCodeDomain> T>
class StatusCode<T> : public detail::StatusCodeStorage<T> {
private:
    using Base = detail::StatusCodeStorage<T>;

public:
    using Domain = T;
    using Value = meta::StatusCodeDomainValue<Domain>;

    StatusCode() = default;
    StatusCode(StatusCode const&) = default;
    StatusCode(StatusCode&&) = default;

    template<typename U, typename... Args>
    requires(!concepts::DecaySameAs<U, StatusCode> && !concepts::DecaySameAs<U, InPlace> &&
             concepts::ConvertibleToStatusCode<StatusCode, U, Args...>)
    constexpr StatusCode(U&& v, Args&&... args)
        : StatusCode(into_status_code(util::forward<U>(v), util::forward<Args>(args)...)) {}

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr explicit StatusCode(InPlace, Args&&... args)
        : Base(in_place, util::addressof(Domain::get()), util::forward<Args>(args)...) {}

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<Value, std::initializer_list<U>, Args...>)
    constexpr explicit StatusCode(InPlace, std::initializer_list<U> list, Args&&... args)
        : Base(in_place, util::addressof(Domain::get()), list, util::forward<Args>(args)...) {}

    constexpr explicit StatusCode(Value const& value)
    requires(concepts::CopyConstructible<Value>)
        : Base(in_place, util::addressof(Domain::get()), value) {}

    constexpr explicit StatusCode(Value&& value) : Base(in_place, util::addressof(Domain::get()), util::move(value)) {}

    StatusCode& operator=(StatusCode const&) = default;
    StatusCode& operator=(StatusCode&&) = default;

    ~StatusCode() = default;

    constexpr auto message() const {
        if (!this->empty()) {
            return this->domain().do_message(*this);
        }
        return container::ErasedString(u8"[invalid status code]");
    }

private:
    template<typename Domain>
    friend class StatusCode;
};
}
