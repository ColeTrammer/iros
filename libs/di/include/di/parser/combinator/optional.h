#pragma once

#include <di/concepts/decay_constructible.h>
#include <di/parser/concepts/parser.h>
#include <di/parser/concepts/parser_context.h>
#include <di/parser/meta/parser_context_result.h>
#include <di/parser/meta/parser_value.h>
#include <di/parser/parser_base.h>
#include <di/vocab/optional/prelude.h>

namespace di::parser {
namespace detail {
    template<typename Parser>
    class OptionalParser : public ParserBase<OptionalParser<Parser>> {
    public:
        template<typename P>
        constexpr explicit OptionalParser(InPlace, P&& parser) : m_parser(util::forward<P>(parser)) {}

        template<concepts::ParserContext Context, typename Value = meta::ParserValue<Context, Parser>>
        requires(concepts::Parser<Parser, Context>)
        constexpr auto parse(Context& context) const -> meta::ParserContextResult<Optional<Value>, Context> {
            auto begin_save = container::begin(context);
            return (m_parser.parse(context) | if_error([&](auto&&) {
                        context.advance(begin_save);
                    }))
                .optional_value();
        }

    private:
        Parser m_parser;
    };

    struct OptionalFunction {
        template<concepts::DecayConstructible Parser>
        constexpr auto operator()(Parser&& parser) const {
            return OptionalParser<meta::Decay<Parser>>(in_place, util::forward<Parser>(parser));
        }
    };
}

constexpr inline auto optional = detail::OptionalFunction {};

template<concepts::DecayConstructible Parser>
requires(concepts::DerivedFrom<Parser, ParserBase<Parser>>)
constexpr auto operator-(Parser&& parser) {
    return optional(util::forward<Parser>(parser));
}
}
