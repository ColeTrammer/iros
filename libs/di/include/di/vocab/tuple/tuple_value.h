#pragma once

#include "di/types/size_t.h"
#include "di/util/declval.h"
#include "di/util/get.h"
#include "di/vocab/tuple/tuple_like.h"

namespace di::meta {
template<concepts::TupleLike Tup, types::size_t index>
using TupleValue = decltype(util::get<index>(util::declval<Tup>()));
}
