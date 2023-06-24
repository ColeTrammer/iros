#pragma once

#include <di/meta/relation.h>
#include <di/parser/basic/code_point_parser.h>
#include <di/parser/combinator/and_then.h>
#include <di/parser/concepts/parser.h>
#include <di/parser/concepts/parser_context.h>
#include <di/vocab/expected/prelude.h>

namespace di::parser {
namespace detail {
    struct MatchOneFunction {
        template<concepts::Predicate<c32> Pred>
        requires(concepts::DecayConstructible<Pred>)
        constexpr auto operator()(Pred&& predicate) const {
            return code_point() << [predicate = auto(util::forward<Pred>(predicate))]<concepts::ParserContext Context>(
                                       Context& context, c32 code_point) -> meta::ParserContextResult<c32, Context> {
                if (!predicate(code_point)) {
                    return Unexpected(context.make_error());
                }
                return code_point;
            };
        }
    };
}

constexpr inline auto match_one = detail::MatchOneFunction {};
}
