#pragma once

#include <di/parser/concepts/parser_context.h>

namespace di::meta {
template<concepts::ParserContext Context>
using ParserContextError = Context::Error;
}
