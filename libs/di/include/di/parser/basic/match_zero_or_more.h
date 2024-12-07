#pragma once

#include "di/container/interface/prelude.h"
#include "di/container/meta/prelude.h"
#include "di/meta/relation.h"
#include "di/parser/concepts/parser_context.h"
#include "di/parser/create_parser.h"
#include "di/parser/meta/parser_context_result.h"
#include "di/parser/parser_base.h"
#include "di/vocab/expected/prelude.h"

namespace di::parser {
namespace detail {
    template<concepts::Predicate<c32> Pred>
    class MatchZeroOrMoreParser : public ParserBase<MatchZeroOrMoreParser<Pred>> {
    public:
        template<typename P>
        constexpr explicit MatchZeroOrMoreParser(InPlace, P&& predicate) : m_predicate(util::forward<P>(predicate)) {}

        template<concepts::ParserContext Context>
        constexpr auto parse(Context& context) const -> meta::ParserContextResult<
            meta::Reconstructed<Context, meta::ContainerIterator<Context>, meta::ContainerIterator<Context>>, Context> {
            auto start = container::begin(context);
            auto sent = container::end(context);

            auto it = start;
            while (it != sent && m_predicate(*it)) {
                ++it;
            }

            context.advance(it);
            return container::reconstruct(in_place_type<Context>, start, it);
        }

    private:
        Pred m_predicate;
    };

    struct MatchZeroOrMoreFunction {
        template<concepts::Predicate<c32> Pred>
        requires(concepts::DecayConstructible<Pred>)
        constexpr auto operator()(Pred&& predicate) const {
            return MatchZeroOrMoreParser<meta::Decay<Pred>> { in_place, util::forward<Pred>(predicate) };
        }
    };
}

constexpr inline auto match_zero_or_more = detail::MatchZeroOrMoreFunction {};
}
