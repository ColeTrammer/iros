#pragma once

#include <di/function/monad/monad_operator.h>
#include <di/vocab/expected/as_fallible.h>
#include <di/vocab/expected/expected.h>
#include <di/vocab/expected/expected_forward_declaration.h>
#include <di/vocab/expected/expected_void_error.h>
#include <di/vocab/expected/expected_void_value.h>
#include <di/vocab/expected/expected_void_void.h>
#include <di/vocab/expected/if_error.h>
#include <di/vocab/expected/if_success.h>
#include <di/vocab/expected/invoke_as_fallible.h>
#include <di/vocab/expected/try_infallible.h>
#include <di/vocab/expected/unexpect.h>
#include <di/vocab/expected/unexpected.h>

namespace di {
using types::unexpect;

using vocab::Expected;
using vocab::Unexpected;

using vocab::as_fallible;
using vocab::if_error;
using vocab::if_success;
using vocab::invoke_as_fallible;
using vocab::try_infallible;
}
