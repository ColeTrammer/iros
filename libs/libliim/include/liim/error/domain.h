#pragma once

#include <liim/forward.h>

namespace LIIM::Error {
class ErrorDomain {
public:
    constexpr ErrorDomain() = default;
    constexpr virtual ~ErrorDomain() {}

    virtual void destroy_error(ErrorTransport<>& value) const = 0;
    virtual StringView message(const ErrorTransport<>& value) const = 0;
    constexpr virtual StringView type() const = 0;
};
}