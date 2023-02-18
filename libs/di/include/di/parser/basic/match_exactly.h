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
    template<concepts::Predicate<c32> Pred>
    class MatchExactly : public ParserBase<MatchExactly<Pred>> {
    public:
        template<typename P>
        constexpr explicit MatchExactly(InPlace, P&& predicate, usize count)
            : m_predicate(util::forward<P>(predicate)), m_count(count) {}

        template<concepts::ParserContext Context>
        constexpr auto parse(Context& context) const -> meta::ParserContextResult<
            meta::Reconstructed<Context, meta::ContainerIterator<Context>, meta::ContainerIterator<Context>>, Context> {
            auto start = container::begin(context);
            auto sent = container::end(context);

            auto remaining = m_count;
            auto it = start;
            while (it != sent && m_predicate(*it) && remaining > 0) {
                ++it;
                --remaining;
            }

            if (remaining > 0) {
                return Unexpected(context.make_error());
            }

            context.advance(it);
            return container::reconstruct(in_place_type<Context>, start, it);
        }

    private:
        Pred m_predicate;
        usize m_count { 0 };
    };

    struct MatchExactlyFunction {
        template<concepts::Predicate<c32> Pred>
        requires(concepts::DecayConstructible<Pred>)
        constexpr auto operator()(Pred&& predicate, usize count) const {
            return MatchExactly<meta::Decay<Pred>> { in_place, util::forward<Pred>(predicate), count };
        }
    };
}

constexpr inline auto match_exactly = detail::MatchExactlyFunction {};
}