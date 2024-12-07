#pragma once

#include "di/container/interface/prelude.h"
#include "di/parser/concepts/parser_context.h"
#include "di/parser/create_parser.h"
#include "di/parser/meta/parser_context_result.h"
#include "di/parser/parser_base.h"
#include "di/vocab/expected/prelude.h"

namespace di::parser {
namespace detail {
    class CodePointParser : public ParserBase<CodePointParser> {
    public:
        template<concepts::ParserContext Context>
        constexpr auto parse(Context& context) const -> meta::ParserContextResult<c32, Context> {
            if (container::empty(context)) {
                return vocab::Unexpected(context.make_error());
            }
            auto it = container::begin(context);
            auto result = *it;
            context.advance(++it);
            return result;
        }
    };

    struct CodePointFunction {
        constexpr auto operator()() const { return CodePointParser {}; }
    };

    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<c32>, concepts::ParserContext auto&) {
        return CodePointParser {};
    }
}

constexpr inline auto code_point = detail::CodePointFunction {};
}
