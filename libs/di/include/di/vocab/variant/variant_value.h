#pragma once

#include <di/types/prelude.h>
#include <di/util/declval.h>
#include <di/util/get.h>
#include <di/vocab/variant/variant_like.h>

namespace di::meta {
template<concepts::VariantLike Var, size_t index>
using VariantValue = decltype(util::get<index>(util::declval<Var>()));
}
