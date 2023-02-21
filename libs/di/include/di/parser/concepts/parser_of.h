#pragma once

#include <di/parser/concepts/parser.h>
#include <di/parser/meta/parser_value.h>

namespace di::concepts {
template<typename Parser, typename Value, typename Context>
concept ParserOf = concepts::Parser<Parser, Context> && SameAs<Value, meta::ParserValue<Parser, Context>>;
}
