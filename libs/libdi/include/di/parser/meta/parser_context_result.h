#pragma once

#include <di/parser/meta/parser_context_error.h>
#include <di/vocab/expected/expected_forward_declaration.h>

namespace di::meta {
template<typename T, concepts::ParserContext Context>
using ParserContextResult = vocab::Expected<T, meta::ParserContextError<Context>>;
}