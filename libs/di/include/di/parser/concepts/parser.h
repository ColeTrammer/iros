#pragma once

#include <di/meta/operations.h>
#include <di/meta/vocab.h>
#include <di/parser/concepts/parser_context.h>
#include <di/parser/parser_base.h>

namespace di::concepts {
template<typename T, typename Context>
concept Parser =
    ParserContext<Context> && DerivedFrom<T, parser::ParserBase<T>> && requires(T& parser, Context& context) {
        { parser.parse(context) } -> ExpectedError<typename Context::Error>;
    };
}
