#pragma once

#include <di/util/declval.h>
#include <di/util/forward.h>
#include <di/util/forward_like.h>
#include <di/util/move.h>
#include <di/util/tag_invoke.h>
#include <di/util/types.h>

namespace di::util::meta {}
namespace di::util::concepts {}

namespace di {
namespace meta = di::util::meta;
namespace conc = di::util::concepts;

using util::declval;
using util::forward;
using util::forward_like;
using util::move;
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
