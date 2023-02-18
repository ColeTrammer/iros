#pragma once

#include <di/util/addressof.h>
#include <di/util/as_const.h>
#include <di/util/as_const_pointer.h>
#include <di/util/badge.h>
#include <di/util/bit_cast.h>
#include <di/util/bitwise_enum.h>
#include <di/util/black_box.h>
#include <di/util/clamp.h>
#include <di/util/clone.h>
#include <di/util/compile_time_fail.h>
#include <di/util/coroutine.h>
#include <di/util/create.h>
#include <di/util/declval.h>
#include <di/util/defer_construct.h>
#include <di/util/exchange.h>
#include <di/util/forward.h>
#include <di/util/get.h>
#include <di/util/immovable.h>
#include <di/util/initializer_list.h>
#include <di/util/is_constant_evaluated.h>
#include <di/util/maybe_clone.h>
#include <di/util/movable_box.h>
#include <di/util/move.h>
#include <di/util/preprocessor.h>
#include <di/util/rebindable_box.h>
#include <di/util/reference_wrapper.h>
#include <di/util/scope_exit.h>
#include <di/util/source_location.h>
#include <di/util/swap.h>
#include <di/util/to_owned.h>
#include <di/util/to_uintptr.h>
#include <di/util/to_underlying.h>
#include <di/util/unreachable.h>
#include <di/util/uuid.h>
#include <di/util/voidify.h>

namespace di {
using util::bit_cast;
using util::is_constant_evaluated;

using util::clone;
using util::create;
using util::maybe_clone;
using util::to_owned;

using util::cref;
using util::ref;
using util::ReferenceWrapper;

using util::InitializerList;

using util::Badge;
using util::DeferConstruct;
using util::Immovable;
using util::MovableBox;
using util::RebindableBox;
using util::ScopeExit;
using util::UUID;

using util::SourceLocation;

using util::addressof;
using util::as_const;
using util::as_const_pointer;
using util::black_box;
using util::clamp;
using util::compile_time_fail;
using util::declval;
using util::exchange;
using util::forward;
using util::generate_uuid;
using util::get;
using util::move;
using util::swap;
using util::to_uintptr;
using util::to_underlying;
using util::unreachable;
using util::voidify;
}
