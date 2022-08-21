#pragma once

#include <di/util/declval.h>
#include <di/util/exchange.h>
#include <di/util/forward.h>
#include <di/util/invoke.h>
#include <di/util/move.h>
#include <di/util/reference_wrapper.h>
#include <di/util/swap.h>
#include <di/util/types.h>

namespace di::util::meta {}
namespace di::util::concepts {}

namespace di {
namespace meta = di::util::meta;
namespace conc = di::util::concepts;

using util::ReferenceWrapper;
using util::Tag;

using util::cref;
using util::declval;
using util::exchange;
using util::forward;
using util::invoke;
using util::invoke_r;
using util::move;
using util::ref;
using util::swap;
using util::tag_invoke;
}

using di::util::i16;
using di::util::i32;
using di::util::i64;
using di::util::i8;
using di::util::u16;
using di::util::u32;
using di::util::u64;
using di::util::u8;
