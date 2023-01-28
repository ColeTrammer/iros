#pragma once

#include <liim/container/erased_string.h>
#include <liim/error/error.h>
#include <liim/error/transport.h>

namespace LIIM::Error {
template<typename Self, Erasable Err>
class TypedErrorDomain : public ErrorDomain {
public:
    using ErrorType = Err;
    using Error = LIIM::Error::Error<ErrorType, Self>;

    constexpr virtual ~TypedErrorDomain() {}
    virtual void destroy_error(ErrorTransport<>& value) const override { error_transport_cast<ErrorType>(value).value.~ErrorType(); }
    virtual ErasedString message(const ErrorTransport<>& value) const override {
        return static_cast<const Self&>(*this).message(error_transport_cast<ErrorType>(value).value);
    }
};
}

using LIIM::Error::TypedErrorDomain;
