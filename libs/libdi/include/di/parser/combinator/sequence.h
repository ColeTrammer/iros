#pragma once

#include <di/parser/concepts/parser.h>
#include <di/parser/concepts/parser_context.h>
#include <di/vocab/prelude.h>

namespace di::parser {
namespace detail {
    template<typename... Parsers>
    class SequenceParser : public ParserBase<SequenceParser<Parsers...>> {
    public:
        constexpr explicit SequenceParser(Parsers&&... parsers) : m_parsers(util::forward<Parsers>(parsers)...) {}

        template<concepts::ParserContext Context>
        requires(concepts::Conjunction<concepts::Parser<Parsers, Context>...>)
        constexpr auto parse(Context& context) const -> meta::ParserContextResult<char32_t, Context> {
            return util::get<0>(m_parsers).parse(context).and_then([&](char32_t code_point) {
                return util::get<1>(m_parsers).parse(context).transform([&] {
                    return code_point;
                });
            });
        }

    private:
        Tuple<Parsers...> m_parsers;
    };

    struct SequenceFunction {
        template<typename... Parsers>
        constexpr auto operator()(Parsers&&... parsers) const {
            return SequenceParser<meta::UnwrapRefDecay<Parsers>...>(util::forward<Parsers>(parsers)...);
        }
    };
}

constexpr inline auto sequence = detail::SequenceFunction {};
}