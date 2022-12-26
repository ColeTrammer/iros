#pragma once

#include <di/container/string/fixed_string.h>
#include <di/meta/size_constant.h>
#include <di/parser/basic/integer.h>
#include <di/parser/parse_unchecked.h>
#include <di/vocab/array/prelude.h>

namespace di {
inline namespace literals {
    inline namespace integral_constant_literals {
        template<char... chars>
        constexpr auto operator""_zic() {
            constexpr auto do_parse = [] {
                auto s = Array { chars... };
                auto view = container::TransparentStringView { s.data(), s.data() + s.size() };
                return parser::parse_unchecked<size_t>(view);
            };
            return meta::SizeConstant<do_parse()> {};
        }
    }
}
}