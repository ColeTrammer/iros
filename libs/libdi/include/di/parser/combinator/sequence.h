#pragma once

#include <di/function/overload.h>
#include <di/function/ycombinator.h>
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
        constexpr auto parse(Context& context) const {
            constexpr auto make_result = function::overload(
                [] {
                    return meta::ParserContextResult<void, Context> {};
                },
                []<typename T>(T&& value) {
                    return meta::ParserContextResult<T, Context>(in_place, util::forward<T>(value));
                },
                []<typename... Types>(Types&&... values) {
                    return meta::ParserContextResult<Tuple<Types...>, Context>(in_place, util::forward<Types>(values)...);
                });

            auto process_index = function::ycombinator([&]<size_t index, typename... Types>(auto& self, InPlaceIndex<index>,
                                                                                            Types&&... values) {
                if constexpr (index >= sizeof...(Parsers)) {
                    return make_result(util::forward<Types>(values)...);
                } else if constexpr (concepts::LanguageVoid<meta::ExpectedValue<decltype(util::get<index>(m_parsers).parse(context))>>) {
                    return util::get<index>(m_parsers).parse(context).and_then([&] {
                        return self(in_place_index<index + 1>, util::forward<Types>(values)...);
                    });
                } else {
                    return util::get<index>(m_parsers).parse(context).and_then([&]<typename T>(T&& value) {
                        return self(in_place_index<index + 1>, util::forward<Types>(values)..., util::forward<T>(value));
                    });
                }
            });

            return process_index(in_place_index<0>);
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