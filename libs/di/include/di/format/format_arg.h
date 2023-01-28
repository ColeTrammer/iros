#pragma once

#include <di/format/concepts/formattable.h>
#include <di/vocab/variant/prelude.h>

namespace di::format {
template<concepts::Formattable... Types>
using FormatArg = Variant<Types&..., Void>;
}