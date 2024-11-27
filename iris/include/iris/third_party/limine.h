#pragma once

#include <di/platform/compiler.h>
#include <di/types/prelude.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// Prevent limine from trying to include system headers. Fake the important
// symbols instead.
#define _LIBCPP_STDINT_H
#define _GCC_WRAP_STDINT_H
#define _STDINT_H

using uint8_t = u8;
using uint16_t = u16;
using uint32_t = u32;
using uint64_t = u64;

using int8_t = i8;
using int16_t = i16;
using int32_t = i32;
using int64_t = i64;

#include <limine.h>

#pragma GCC diagnostic pop
