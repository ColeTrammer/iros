#pragma once

#include <di/util/as_const.h>
#include <di/util/declval.h>
#include <di/util/exchange.h>
#include <di/util/forward.h>
#include <di/util/invoke.h>
#include <di/util/move.h>
#include <di/util/reference_wrapper.h>
#include <di/util/swap.h>
#include <di/util/types.h>
#include <di/util/unreachable.h>

namespace di::util::meta {}
namespace di::util::concepts {}

namespace di {
namespace meta = di::util::meta;
namespace conc = di::util::concepts;

using util::in_place;
using util::in_place_index;
using util::in_place_type;
using util::piecewise_construct;
using util::ptrdiff_t;
using util::ReferenceWrapper;
using util::size_t;
using util::Tag;

using util::as_const;
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
using util::unreachable;
}
