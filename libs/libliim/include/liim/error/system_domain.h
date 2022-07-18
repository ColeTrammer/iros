#pragma once

#include <errno.h>
#include <liim/error/typed_domain.h>

namespace LIIM::Error {
class SystemErrorDomain : public TypedErrorDomain<SystemErrorDomain, int> {
public:
    constexpr virtual ~SystemErrorDomain() {}
    StringView message(int code) const { return strerror(code); }
    constexpr virtual StringView type() const override { return "SystemError"; }
};

using SystemError = SystemErrorDomain::Error;

constexpr SystemErrorDomain system_error_domain = SystemErrorDomain {};

constexpr SystemError make_system_error(int code) {
    return SystemError(move(code), &system_error_domain);
}

inline SystemError make_system_error_from_errno() {
    return make_system_error(errno);
}
}

using LIIM::Error::make_system_error;
using LIIM::Error::make_system_error_from_errno;
using LIIM::Error::SystemError;
