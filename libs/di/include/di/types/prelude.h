#pragma once

#include <di/types/byte.h>
#include <di/types/char.h>
#include <di/types/in_place.h>
#include <di/types/in_place_index.h>
#include <di/types/in_place_template.h>
#include <di/types/in_place_type.h>
#include <di/types/integers.h>
#include <di/types/intptr_t.h>
#include <di/types/nontype.h>
#include <di/types/nullptr_t.h>
#include <di/types/partial_ordering.h>
#include <di/types/piecewise_construct.h>
#include <di/types/ptrdiff_t.h>
#include <di/types/size_t.h>
#include <di/types/ssize_t.h>
#include <di/types/strong_ordering.h>
#include <di/types/uintptr_t.h>
#include <di/types/void.h>
#include <di/types/weak_ordering.h>

namespace di {
using types::Byte;
using types::byte;
using types::to_integer;

using types::in_place;
using types::in_place_index;
using types::in_place_template;
using types::in_place_type;
using types::nontype;
using types::piecewise_construct;

using types::InPlace;
using types::InPlaceIndex;
using types::InPlaceTemplate;
using types::InPlaceType;
using types::Nontype;
using types::PiecewiseConstruct;

using types::Void;

using types::intmax_t;
using types::intptr_t;
using types::nullptr_t;
using types::ptrdiff_t;
using types::size_t;
using types::ssize_t;
using types::uintmax_t;
using types::uintptr_t;

using types::partial_ordering;
using types::strong_ordering;
using types::weak_ordering;

#ifdef DI_HAVE_128_BIT_INTEGERS
using types::i128;
using types::u128;
#endif

using types::i16;
using types::i32;
using types::i64;
using types::i8;
using types::u16;
using types::u32;
using types::u64;
using types::u8;

using types::imax;
using types::iptr;
using types::isize;
using types::umax;
using types::uptr;
using types::usize;

using types::c16;
using types::c32;
using types::c8;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_TYPES)
using di::byte;
using di::c16;
using di::c32;
using di::c8;
using di::i16;
using di::i32;
using di::i64;
using di::i8;
using di::imax;
using di::intptr_t;
using di::iptr;
using di::isize;
using di::ptrdiff_t;
using di::size_t;
using di::ssize_t;
using di::u16;
using di::u32;
using di::u64;
using di::u8;
using di::uintptr_t;
using di::umax;
using di::uptr;
using di::usize;

#ifdef DI_HAVE_128_BIT_INTEGERS
using di::i128;
using di::u128;
#endif
#endif
