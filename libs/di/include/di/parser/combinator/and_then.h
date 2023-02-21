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
        template<typename P, typename F>
        constexpr explicit AndThenParser(InPlace, P&& parser, F&& function)
            : m_parser(util::forward<P>(parser)), m_function(util::forward<F>(function)) {}

        template<concepts::ParserContext Context, typename Value = meta::ParserValue<Context, Parser>>
        requires(concepts::Parser<Parser, Context>)
        constexpr auto parse(Context& context) const {
            auto begin_save = container::begin(context);
            if constexpr (concepts::LanguageVoid<Value>) {
                return m_parser.parse(context).and_then([&] {
                    return m_function(context) | if_error([&](auto&&) {
                               context.advance(begin_save);
                           });
                });
            } else {
                return m_parser.parse(context).and_then([&](Value&& value) {
                    return m_function(context, util::forward<Value>(value)) | if_error([&](auto&&) {
                               context.advance(begin_save);
                           });
                });
            }
        }

    private:
        Parser m_parser;
        Fun m_function;
    };

    struct AndThenFunction {
        template<concepts::DecayConstructible Parser, concepts::DecayConstructible Fun>
        constexpr auto operator()(Parser&& parser, Fun&& function) const {
            return AndThenParser<meta::Decay<Parser>, meta::Decay<Fun>>(in_place, util::forward<Parser>(parser),
                                                                        util::forward<Fun>(function));
        }
    };
}

constexpr inline auto and_then = detail::AndThenFunction {};

template<concepts::DecayConstructible Parser, concepts::DecayConstructible Fun>
requires(concepts::DerivedFrom<Parser, ParserBase<Parser>>)
constexpr auto operator<<(Parser&& parser, Fun&& function) {
    return and_then(util::forward<Parser>(parser), util::forward<Fun>(function));
}
}
