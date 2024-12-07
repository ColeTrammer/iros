#pragma once

#include "di/container/interface/prelude.h"
#include "di/parser/concepts/parser_context.h"
#include "di/parser/meta/parser_context_result.h"
#include "di/parser/parser_base.h"
#include "di/vocab/expected/prelude.h"

namespace di::parser {
namespace detail {
    class EofParser : public ParserBase<EofParser> {
    public:
        template<concepts::ParserContext Context>
        constexpr auto parse(Context& context) const -> meta::ParserContextResult<void, Context> {
            if (!container::empty(context)) {
                return vocab::Unexpected(context.make_error());
            }
            return {};
        }
    };

    struct EofFunction {
        constexpr auto operator()() const { return EofParser {}; }
    };
}

constexpr inline auto eof = detail::EofFunction {};
}
