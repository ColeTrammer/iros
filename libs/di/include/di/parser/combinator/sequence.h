#pragma once

#include <di/function/overload.h>
#include <di/function/ycombinator.h>
#include <di/parser/concepts/parser.h>
#include <di/parser/concepts/parser_context.h>
#include <di/vocab/prelude.h>

namespace di::parser {
namespace detail {
    struct SequenceParserMarker {};

    template<typename... Parsers>
    class SequenceParser
        : public ParserBase<SequenceParser<Parsers...>>
        , public SequenceParserMarker {
    public:
        template<typename... Ps>
        constexpr explicit SequenceParser(InPlace, Ps&&... parsers) : m_parsers(util::forward<Ps>(parsers)...) {}

        template<concepts::ParserContext Context>
        requires(concepts::Conjunction<concepts::Parser<Parsers, Context>...>)
        constexpr auto parse(Context& context) const {
            constexpr auto make_result = function::overload(
                [] {
                    return meta::ParserContextResult<void, Context> {};
                },
                []<typename T>(T&& value) {
                    return meta::ParserContextResult<T, Context>(in_place, util::forward<T>(value));
                },
                []<typename... Types>(Types&&... values) {
                    return meta::ParserContextResult<Tuple<Types...>, Context>(in_place,
                                                                               util::forward<Types>(values)...);
                });

            auto process_index = function::ycombinator(
                [&]<size_t index, typename... Types>(auto& self, InPlaceIndex<index>, Types&&... values) {
                    if constexpr (index >= sizeof...(Parsers)) {
                        return make_result(util::forward<Types>(values)...);
                    } else {
                        using Value = meta::ExpectedValue<
                            decltype(util::get<index>(util::declval<Tuple<Parsers...> const&>()).parse(context))>;
                        if constexpr (concepts::LanguageVoid<Value>) {
                            return util::get<index>(m_parsers).parse(context).and_then([&] {
                                return self(in_place_index<index + 1>, util::forward<Types>(values)...);
                            });
                        } else {
                            return util::get<index>(m_parsers).parse(context).and_then([&](Value&& value) {
                                return self(in_place_index<index + 1>, util::forward<Types>(values)...,
                                            util::forward<Value>(value));
                            });
                        }
                    }
                });

            return process_index(in_place_index<0>);
        }

        Tuple<Parsers...> m_parsers;
    };

    struct SequenceFunction {
        template<concepts::DecayConstructible... Parsers>
        constexpr auto operator()(Parsers&&... parsers) const {
            return SequenceParser<meta::Decay<Parsers>...>(in_place, util::forward<Parsers>(parsers)...);
        }
    };
}

constexpr inline auto sequence = detail::SequenceFunction {};

template<concepts::DecayConstructible Left, concepts::DecayConstructible Right>
requires(concepts::DerivedFrom<Left, ParserBase<Left>> && concepts::DerivedFrom<Right, ParserBase<Right>>)
constexpr auto operator>>(Left&& left, Right&& right) {
    return sequence(util::forward<Left>(left), util::forward<Right>(right));
}

template<concepts::DecayConstructible Left, concepts::DecayConstructible Right>
requires(concepts::DerivedFrom<Left, ParserBase<Left>> && concepts::DerivedFrom<Right, ParserBase<Right>> &&
         !concepts::DerivedFrom<Left, detail::SequenceParserMarker> &&
         concepts::DerivedFrom<Right, detail::SequenceParserMarker>)
constexpr auto operator>>(Left&& left, Right&& right) {
    return vocab::apply(
        [&]<typename... Rs>(Rs&&... rs) {
            return sequence(util::forward<Left>(left), util::forward<Rs>(rs)...);
        },
        util::forward<Right>(right).m_parsers);
}

template<concepts::DecayConstructible Left, concepts::DecayConstructible Right>
requires(concepts::DerivedFrom<Left, ParserBase<Left>> && concepts::DerivedFrom<Right, ParserBase<Right>> &&
         concepts::DerivedFrom<Left, detail::SequenceParserMarker> &&
         !concepts::DerivedFrom<Right, detail::SequenceParserMarker>)
constexpr auto operator>>(Left&& left, Right&& right) {
    return vocab::apply(
        [&]<typename... Ls>(Ls&&... ls) {
            return sequence(util::forward<Ls>(ls)..., util::forward<Right>(right));
        },
        util::forward<Left>(left).m_parsers);
}

template<concepts::DecayConstructible Left, concepts::DecayConstructible Right>
requires(concepts::DerivedFrom<Left, ParserBase<Left>> && concepts::DerivedFrom<Right, ParserBase<Right>> &&
         concepts::DerivedFrom<Left, detail::SequenceParserMarker> &&
         concepts::DerivedFrom<Right, detail::SequenceParserMarker>)
constexpr auto operator>>(Left&& left, Right&& right) {
    return vocab::apply(
        [&]<typename... Ls>(Ls&&... ls) {
            return vocab::apply(
                [&]<typename... Rs>(Rs&&... rs) {
                    return sequence(util::forward<Ls>(ls)..., util::forward<Rs>(rs)...);
                },
                util::forward<Right>(right).m_parsers);
        },
        util::forward<Left>(left).m_parsers);
}
}
