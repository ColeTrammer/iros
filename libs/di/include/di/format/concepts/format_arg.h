#pragma once

#include "di/format/format_context.h"
#include "di/vocab/variant/variant_like.h"

namespace di::concepts {
template<typename T>
concept FormatArg = VariantLike<T>;
}
