#pragma once

#include <di/meta/expected_value.h>
#include <di/parser/concepts/parser.h>
#include <di/parser/concepts/parser_context.h>
#include <di/util/declval.h>

namespace di::meta {
template<concepts::ParserContext Context, concepts::Parser<Context> Parser>
using ParserValue = meta::ExpectedValue<decltype(util::declval<Parser&>().parse(util::declval<Context&>()))>;
}
