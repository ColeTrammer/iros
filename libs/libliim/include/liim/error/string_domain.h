#pragma once

#include <liim/error/error.h>
#include <liim/error/typed_domain.h>

namespace LIIM::Error {
class StringErrorDomain : public TypedErrorDomain<StringErrorDomain, UniquePtr<String>> {
public:
    constexpr virtual ~StringErrorDomain() {}
    virtual StringView message(const ErrorType& value) const { return value->view(); }
    constexpr virtual StringView type() const { return "StringError"sv; }
};

using StringError = StringErrorDomain::Error;

constexpr StringErrorDomain string_error_domain = StringErrorDomain {};

template<typename... Args>
static StringError make_string_error(LIIM::Format::FormatString<Args...> pattern, Args&&... args) {
    auto result = vformat(pattern.data(), make_format_args(forward<Args>(args)...));
    return StringError(make_unique<String>(move(result)), &string_error_domain);
}
}

using LIIM::Error::make_string_error;