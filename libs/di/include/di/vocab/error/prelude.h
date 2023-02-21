#pragma once

#include <di/vocab/error/erased_status_code.h>
#include <di/vocab/error/error.h>
#include <di/vocab/error/generic_domain.h>
#include <di/vocab/error/result.h>
#include <di/vocab/error/status_code.h>
#include <di/vocab/error/status_code_domain.h>
#include <di/vocab/error/status_code_equality.h>
#include <di/vocab/error/status_code_storage.h>
#include <di/vocab/error/void_status_code.h>

namespace di {
// The error module is nearly an implementation of P1028R4.
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1028r4.pdf
using vocab::BasicError;
using vocab::Error;
using vocab::GenericCode;
using vocab::Result;
using vocab::StatusCode;
using vocab::StatusCodeDomain;

using vocab::into_status_code;
}
