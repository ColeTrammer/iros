#pragma once

#include <di/parser/concepts/parser.h>
#include <di/parser/concepts/parser_context.h>
#include <di/parser/meta/parser_context_result.h>
#include <di/parser/meta/parser_value.h>
#include <di/parser/parser_base.h>

namespace di::parser {
namespace detail {
    template<typename Parser, typename Fun>
    class TransformParser : public ParserBase<TransformParser<Parser, Fun>> {
    public:
        constexpr explicit TransformParser(Parser&& parser, Fun&& function)
            : m_parser(util::move(parser)), m_function(util::move(function)) {}

        template<concepts::ParserContext Context, typename Value = meta::ParserValue<Context, Parser>>
        requires(concepts::Parser<Parser, Context>)
        constexpr auto parse(Context& context) const {
            return m_parser.parse(context).transform(m_function);
        }

    private:
        Parser m_parser;
        Fun m_function;
    };

    struct TransformFunction {
        template<typename Parser, typename Fun>
        constexpr auto operator()(Parser&& parser, Fun&& function) const {
            return TransformParser<meta::UnwrapRefDecay<Parser>, meta::UnwrapRefDecay<Fun>>(util::forward<Parser>(parser),
                                                                                            util::forward<Fun>(function));
        }
    };
}

constexpr inline auto transform = detail::TransformFunction {};
}