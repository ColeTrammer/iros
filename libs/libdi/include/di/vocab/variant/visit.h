#pragma once

#include <di/vocab/array/array.h>
#include <di/vocab/variant/variant_like.h>

namespace di::vocab {
template<typename Vis, concepts::VariantLike... Var>
constexpr void visit(Vis&&, Var&&...) {}
}
