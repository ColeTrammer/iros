#pragma once

#include <di/concepts/predicate.h>
#include <di/container/interface/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/parser/basic/match_zero_or_more.h>
#include <di/parser/combinator/and_then.h>
#include <di/parser/concepts/parser_context.h>
#include <di/parser/create_parser.h>
#include <di/parser/meta/parser_context_result.h>
#include <di/parser/parser_base.h>
#include <di/vocab/expected/prelude.h>

namespace di::parser {
namespace detail {
    struct MatchOneOrMoreFunction {
        template<concepts::Predicate<c32> Pred>
        requires(concepts::DecayConstructible<Pred>)
        constexpr auto operator()(Pred&& predicate) const {
            return match_zero_or_more(util::forward<Pred>(predicate))
                       << []<concepts::ParserContext Context, typename View>(
                              Context& context, View view) -> meta::ParserContextResult<View, Context> {
                if (container::empty(view)) {
                    return Unexpected(context.make_error());
                }
                return view;
            };
        }
    };
}

constexpr inline auto match_one_or_more = detail::MatchOneOrMoreFunction {};
}
