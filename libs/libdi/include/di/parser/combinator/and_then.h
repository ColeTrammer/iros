#pragma once

#include <di/parser/concepts/parser.h>
#include <di/parser/concepts/parser_context.h>
#include <di/parser/meta/parser_context_result.h>
#include <di/parser/meta/parser_value.h>
#include <di/parser/parser_base.h>

namespace di::parser {
namespace detail {
    template<typename Parser, typename Fun>
    class AndThenParser : public ParserBase<AndThenParser<Parser, Fun>> {
    public:
        constexpr explicit AndThenParser(Parser&& parser, Fun&& function)
            : m_parser(util::move(parser)), m_function(util::move(function)) {}

        template<concepts::ParserContext Context, typename Value = meta::ParserValue<Context, Parser>>
        requires(concepts::Parser<Parser, Context>)
        constexpr auto parse(Context& context) const {
            return m_parser.parse(context).and_then([&]<typename... Types>(Types&&... values) {
                return m_function(context, util::forward<Types>(values)...);
            });
        }

    private:
        Parser m_parser;
        Fun m_function;
    };

    struct AndThenFunction {
        template<typename Parser, typename Fun>
        constexpr auto operator()(Parser&& parser, Fun&& function) const {
            return AndThenParser<meta::UnwrapRefDecay<Parser>, meta::UnwrapRefDecay<Fun>>(util::forward<Parser>(parser),
                                                                                          util::forward<Fun>(function));
        }
    };
}

constexpr inline auto and_then = detail::AndThenFunction {};
}