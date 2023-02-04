#pragma once

#include <di/any/prelude.h>
#include <di/assert/prelude.h>
#include <di/bit/prelude.h>
#include <di/chrono/prelude.h>
#include <di/cli/prelude.h>
#include <di/concepts/prelude.h>
#include <di/container/prelude.h>
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

#ifndef DI_NO_GLOBALS
#define ASSERT        DI_ASSERT
#define ASSERT_EQ     DI_ASSERT_EQ
#define ASSERT_NOT_EQ DI_ASSERT_NOT_EQ
#define ASSERT_LT     DI_ASSERT_LT
#define ASSERT_LT_EQ  DI_ASSERT_LT_EQ
#define ASSERT_GT     DI_ASSERT_GT
#define ASSERT_GT_EQ  DI_ASSERT_GT_EQ

#define TRY DI_TRY

using namespace di::literals;

using di::types::c16;
using di::types::c32;
using di::types::c8;
using di::types::i16;
using di::types::i32;
using di::types::i64;
using di::types::i8;
using di::types::imax;
using di::types::intptr_t;
using di::types::iptr;
using di::types::isize;
using di::types::ptrdiff_t;
using di::types::size_t;
using di::types::ssize_t;
using di::types::u16;
using di::types::u32;
using di::types::u64;
using di::types::u8;
using di::types::uintptr_t;
using di::types::umax;
using di::types::uptr;
using di::types::usize;

#ifdef DI_HAVE_128_BIT_INTEGERS
using di::types::i128;
using di::types::u128;
#endif
#endif
