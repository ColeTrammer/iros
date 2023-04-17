#pragma once

#include <di/any/prelude.h>
#include <di/assert/prelude.h>
#include <di/bit/prelude.h>
#include <di/chrono/prelude.h>
#include <di/cli/prelude.h>
#include <di/concepts/prelude.h>
#include <di/container/prelude.h>
#include <di/exec/prelude.h>
#include <di/execution/prelude.h>
#include <di/format/prelude.h>
#include <di/function/prelude.h>
#include <di/io/prelude.h>
#include <di/math/prelude.h>
#include <di/meta/prelude.h>
#include <di/parser/prelude.h>
#include <di/platform/prelude.h>
#include <di/random/prelude.h>
#include <di/sync/prelude.h>
#include <di/types/prelude.h>
#include <di/util/prelude.h>
#include <di/vocab/prelude.h>

// NOTE: this is included after all other headers because it uses
//       di::format::to_string(), which depends on many types which
//       want to use assertions, like Optional and String.
#include <di/assert/assert_impl.h>
