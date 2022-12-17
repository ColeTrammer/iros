#pragma once

#include <di/concepts/predicate.h>
#include <di/container/interface/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/parser/concepts/parser_context.h>
#include <di/parser/create_parser.h>
#include <di/parser/meta/parser_context_result.h>
#include <di/parser/parser_base.h>
#include <di/vocab/expected/prelude.h>

namespace di::parser {
namespace detail {
    template<concepts::Predicate<char32_t> Pred>
    class MatchOneOrMoreParser : public ParserBase<MatchOneOrMoreParser<Pred>> {
    public:
        template<typename P>
        constexpr explicit MatchOneOrMoreParser(InPlace, P&& predicate) : m_predicate(util::forward<P>(predicate)) {}

        template<concepts::ParserContext Context>
        constexpr auto parse(Context& context) const
            -> meta::ParserContextResult<meta::Reconstructed<Context, meta::ContainerIterator<Context>, meta::ContainerIterator<Context>>,
                                         Context> {
            auto start = container::begin(context);
            auto sent = container::end(context);

            auto it = start;
            while (it != sent && m_predicate(*it)) {
                ++it;
            }

            if (it == start) {
                return Unexpected(context.make_error());
            }

            context.advance(it);
            return container::reconstruct(in_place_type<Context>, start, it);
        }

    private:
        Pred m_predicate;
    };

    struct MatchOneOrMoreFunction {
        template<concepts::Predicate<char32_t> Pred>
        requires(concepts::DecayConstructible<Pred>)
        constexpr auto operator()(Pred&& predicate) const {
            return MatchOneOrMoreParser<meta::Decay<Pred>> { in_place, util::forward<Pred>(predicate) };
        }
    };
}

constexpr inline auto match_one_or_more = detail::MatchOneOrMoreFunction {};
}