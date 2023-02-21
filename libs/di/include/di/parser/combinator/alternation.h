#pragma once

#include <di/function/overload.h>
#include <di/function/ycombinator.h>
#include <di/parser/concepts/parser.h>
#include <di/parser/concepts/parser_context.h>
#include <di/vocab/prelude.h>

namespace di::parser {
namespace detail {
    struct AlternationParserMarker {};

    template<typename... Parsers>
    class AlternationParser
        : public ParserBase<AlternationParser<Parsers...>>
        , public AlternationParserMarker {
    public:
        template<typename... Ps>
        constexpr explicit AlternationParser(InPlace, Ps&&... parsers) : m_parsers(util::forward<Ps>(parsers)...) {}

        template<concepts::ParserContext Context>
        requires(concepts::Conjunction<concepts::Parser<Parsers, Context>...>)
        constexpr auto parse(Context& context) const {
            constexpr bool should_be_void =
                concepts::Conjunction<concepts::LanguageVoid<meta::ParserValue<Context, Parsers>>...>;

            using Result = meta::ParserContextResult<
                meta::Conditional<should_be_void, void,
                                  Variant<meta::Conditional<concepts::LanguageVoid<meta::ParserValue<Context, Parsers>>,
                                                            Void, meta::ParserValue<Context, Parsers>>...>>,
                Context>;

            auto process_index = function::ycombinator([&]<size_t index>(auto& self, InPlaceIndex<index>) -> Result {
                if constexpr (index >= sizeof...(Parsers)) {
                    return Result(Unexpected(context.make_error()));
                } else {
                    using Value = meta::ExpectedValue<
                        decltype(util::get<index>(util::declval<Tuple<Parsers...> const&>()).parse(context))>;
                    auto result = util::get<index>(m_parsers).parse(context);
                    if (!result) {
                        return self(in_place_index<index + 1>);
                    } else if constexpr (concepts::LanguageVoid<Value>) {
                        return Result(in_place, in_place_index<index>);
                    } else {
                        return Result(in_place, in_place_index<index>, util::move(result).value());
                    }
                }
            });

            return process_index(in_place_index<0>);
        }

        Tuple<Parsers...> m_parsers;
    };

    struct AlternationFunction {
        template<concepts::DecayConstructible... Parsers>
        constexpr auto operator()(Parsers&&... parsers) const {
            return AlternationParser<meta::Decay<Parsers>...>(in_place, util::forward<Parsers>(parsers)...);
        }
    };
}

constexpr inline auto alternation = detail::AlternationFunction {};

template<concepts::DecayConstructible Left, concepts::DecayConstructible Right>
requires(concepts::DerivedFrom<Left, ParserBase<Left>> && concepts::DerivedFrom<Right, ParserBase<Right>>)
constexpr auto operator|(Left&& left, Right&& right) {
    return alternation(util::forward<Left>(left), util::forward<Right>(right));
}

template<concepts::DecayConstructible Left, concepts::DecayConstructible Right>
requires(concepts::DerivedFrom<Left, ParserBase<Left>> && concepts::DerivedFrom<Right, ParserBase<Right>> &&
         !concepts::DerivedFrom<Left, detail::AlternationParserMarker> &&
         concepts::DerivedFrom<Right, detail::AlternationParserMarker>)
constexpr auto operator|(Left&& left, Right&& right) {
    return vocab::apply(
        [&]<typename... Rs>(Rs&&... rs) {
            return alternation(util::forward<Left>(left), util::forward<Rs>(rs)...);
        },
        util::forward<Right>(right).m_parsers);
}

template<concepts::DecayConstructible Left, concepts::DecayConstructible Right>
requires(concepts::DerivedFrom<Left, ParserBase<Left>> && concepts::DerivedFrom<Right, ParserBase<Right>> &&
         concepts::DerivedFrom<Left, detail::AlternationParserMarker> &&
         !concepts::DerivedFrom<Right, detail::AlternationParserMarker>)
constexpr auto operator|(Left&& left, Right&& right) {
    return vocab::apply(
        [&]<typename... Ls>(Ls&&... ls) {
            return alternation(util::forward<Ls>(ls)..., util::forward<Right>(right));
        },
        util::forward<Left>(left).m_parsers);
}

template<concepts::DecayConstructible Left, concepts::DecayConstructible Right>
requires(concepts::DerivedFrom<Left, ParserBase<Left>> && concepts::DerivedFrom<Right, ParserBase<Right>> &&
         concepts::DerivedFrom<Left, detail::AlternationParserMarker> &&
         concepts::DerivedFrom<Right, detail::AlternationParserMarker>)
constexpr auto operator|(Left&& left, Right&& right) {
    return vocab::apply(
        [&]<typename... Ls>(Ls&&... ls) {
            return vocab::apply(
                [&]<typename... Rs>(Rs&&... rs) {
                    return alternation(util::forward<Ls>(ls)..., util::forward<Rs>(rs)...);
                },
                util::forward<Right>(right).m_parsers);
        },
        util::forward<Left>(left).m_parsers);
}
}
