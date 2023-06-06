#pragma once

#include <di/container/string/fixed_string.h>
#include <di/meta/constexpr.h>
#include <di/parser/basic/integer.h>
#include <di/parser/parse_unchecked.h>
#include <di/parser/string_view_parser_context.h>
#include <di/vocab/array/prelude.h>

namespace di {
inline namespace literals {
    inline namespace constexpr_integral_literals {
        template<char... chars>
        constexpr auto operator""_zic() {
            constexpr auto do_parse = [] {
                auto s = Array { chars... };
                auto view = container::TransparentStringView { s.data(), s.data() + s.size() };
                return parser::parse_unchecked<size_t>(view);
            };
            return meta::Constexpr<do_parse()> {};
        }
    }
}
}
