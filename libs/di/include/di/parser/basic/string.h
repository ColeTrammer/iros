#pragma once

#include <di/container/path/prelude.h>
#include <di/container/string/prelude.h>
#include <di/parser/basic/match_zero_or_more.h>
#include <di/parser/combinator/optional.h>
#include <di/parser/combinator/sequence.h>
#include <di/parser/integral_set.h>

namespace di::parser {
namespace detail {
    template<concepts::Encoding Enc, concepts::ParserContext Context>
    requires(concepts::SameAs<Enc, meta::Encoding<Context>>)
    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<container::string::StringViewImpl<Enc>>,
                              Context&) {
        return match_zero_or_more([](auto) {
                   return true;
               }) <<
                   [](Context& context,
                      auto results) -> meta::ParserContextResult<container::string::StringViewImpl<Enc>, Context> {
            auto encoding = context.encoding();
            auto begin = encoding::unicode_code_point_unwrap(encoding, results.begin());
            auto end = encoding::unicode_code_point_unwrap(encoding, results.end());
            return container::string::StringViewImpl<Enc> { begin, end, encoding };
        };
    }

    template<concepts::Encoding Enc, concepts::ParserContext Context>
    requires(concepts::SameAs<Enc, meta::Encoding<Context>>)
    constexpr auto tag_invoke(types::Tag<create_parser_in_place>, InPlaceType<container::PathViewImpl<Enc>>, Context&) {
        return match_zero_or_more([](auto) {
                   return true;
               }) <<
                   [](Context& context,
                      auto results) -> meta::ParserContextResult<container::PathViewImpl<Enc>, Context> {
            auto encoding = context.encoding();
            auto begin = encoding::unicode_code_point_unwrap(encoding, results.begin());
            auto end = encoding::unicode_code_point_unwrap(encoding, results.end());
            auto view = container::string::StringViewImpl<Enc> { begin, end, encoding };
            return container::PathViewImpl<Enc> { view };
        };
    }
}
}