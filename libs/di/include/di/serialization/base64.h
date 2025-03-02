#pragma once

#include "di/container/concepts/container_of.h"
#include "di/container/interface/begin.h"
#include "di/container/iterator/distance.h"
#include "di/container/string/string_view.h"
#include "di/container/vector/prelude.h"
#include "di/container/view/chunk.h"
#include "di/container/view/enumerate.h"
#include "di/container/view/range.h"
#include "di/format/prelude.h"
#include "di/parser/basic/match_zero_or_more.h"
#include "di/parser/prelude.h"
#include "di/vocab/span/prelude.h"

namespace di::serialization {
template<concepts::ContainerOf<byte> Con = di::Vector<byte>>
class Base64 {
public:
    Base64() = default;

    constexpr explicit Base64(Con container) : m_container(di::move(container)) {}

    constexpr auto container() const& -> Con const& { return m_container; }
    constexpr auto container() && -> Con&& { return di::move(m_container); }

    auto operator==(Base64 const& other) const -> bool = default;

private:
    constexpr static auto alphabet() {
        auto result = di::Array<u8, 256> {};
        result.fill(255);

        auto v = u8(0);
        for (auto ch : range('A', 'Z' + 1)) {
            result[ch] = v++;
        }
        for (auto ch : range('a', 'z' + 1)) {
            result[ch] = v++;
        }
        for (auto ch : range('0', '9' + 1)) {
            result[ch] = v++;
        }
        result['+'] = v++;
        result['/'] = v++;
        result['='] = 0;
        return result;
    }

    constexpr static auto reverse_alphabet() {
        auto result = di::Array<char, 64> {};
        for (auto [i, v] : di::enumerate(alphabet())) {
            if (v != 255) {
                result[v] = char(i);
            }
        }
        return result;
    }

    constexpr static auto value_to_base64_digit(u8 value) -> char { return reverse_alphabet()[value]; }

    constexpr static auto base64_digit_to_value(char value) -> u8 { return alphabet()[value]; }

    template<concepts::Encoding Enc>
    constexpr friend auto tag_invoke(types::Tag<format::formatter_in_place>, InPlaceType<Base64>,
                                     FormatParseContext<Enc>&) {
        auto do_output = [](concepts::FormatContext auto& context,
                            concepts::DecaySameAs<Base64> auto&& base64) -> Result<> {
            for (auto&& range : di::chunk(base64.m_container, 3)) {
                auto it = begin(range);
                switch (di::distance(range)) {
                    case 3: {
                        auto b0 = u8(*it);
                        auto b1 = u8(*++it);
                        auto b2 = u8(*++it);

                        auto v0 = ((b0 & 0xfc) >> 2);
                        auto v1 = ((b0 & 0x03) << 4) | ((b1 & 0xf0) >> 4);
                        auto v2 = ((b1 & 0x0f) << 2) | ((b2 & 0xc0) >> 6);
                        auto v3 = ((b2 & 0x3f) << 0);
                        (void) context.output(value_to_base64_digit(v0));
                        (void) context.output(value_to_base64_digit(v1));
                        (void) context.output(value_to_base64_digit(v2));
                        (void) context.output(value_to_base64_digit(v3));
                        break;
                    }
                    case 2: {
                        auto b0 = u8(*it);
                        auto b1 = u8(*++it);

                        auto v0 = ((b0 & 0xfc) >> 2);
                        auto v1 = ((b0 & 0x03) << 4) | ((b1 & 0xf0) >> 4);
                        auto v2 = ((b1 & 0x0f) << 2);
                        (void) context.output(value_to_base64_digit(v0));
                        (void) context.output(value_to_base64_digit(v1));
                        (void) context.output(value_to_base64_digit(v2));
                        (void) context.output('=');
                        break;
                    }
                    case 1: {
                        auto b0 = u8(*it);

                        auto v0 = (b0 & 0xfc) >> 2;
                        auto v1 = (b0 & 0x03) << 4;
                        (void) context.output(value_to_base64_digit(v0));
                        (void) context.output(value_to_base64_digit(v1));
                        (void) context.output('=');
                        (void) context.output('=');
                    }
                    default:
                        break;
                }
            }
            return {};
        };
        return Result<decltype(do_output)>(di::move(do_output));
    }

    constexpr friend auto tag_invoke(types::Tag<parser::create_parser_in_place>, InPlaceType<Base64>) {
        auto valid_base64 = ('0'_m - '9'_m || 'a'_m - 'z'_m || 'A'_m - 'Z'_m || '+'_m || '/'_m || '='_m);

        return (parser::match_zero_or_more(valid_base64))
                   << []<typename Context>(
                          Context& context,
                          concepts::CopyConstructible auto results) -> meta::ParserContextResult<Base64, Context> {
            auto result = Base64 {};
            auto got_equal = false;
            for (auto group : chunk(results, 4)) {
                if (distance(group) != 4) {
                    return Unexpected(context.make_error());
                }

                auto it = begin(group);
                auto c0 = *it;
                if (c0 == '=') {
                    return Unexpected(context.make_error());
                }
                auto c1 = *++it;
                if (c1 == '=') {
                    return Unexpected(context.make_error());
                }
                auto c2 = *++it;
                if (got_equal && c2 != '=') {
                    return Unexpected(context.make_error());
                }
                got_equal |= c2 == '=';
                auto c3 = *++it;
                if (got_equal && c3 != '=') {
                    return Unexpected(context.make_error());
                }
                got_equal |= c3 == '=';

                auto v0 = base64_digit_to_value(char(c0));
                auto v1 = base64_digit_to_value(char(c1));
                auto v2 = base64_digit_to_value(char(c2));
                auto v3 = base64_digit_to_value(char(c3));

                auto b0 = ((v0 & 0x3f) << 2) | ((v1 & 0x30) >> 4);
                auto b1 = ((v1 & 0x0f) << 4) | ((v2 & 0x3c) >> 2);
                auto b2 = ((v2 & 0x03) << 6) | ((v3 & 0x3f) >> 0);

                result.m_container.push_back(byte(b0));
                if (c2 != '=') {
                    result.m_container.push_back(byte(b1));
                }
                if (c3 != '=') {
                    result.m_container.push_back(byte(b2));
                }
            }
            return result;
        };
    }

    Con m_container;
};
}

namespace di {
inline namespace literals {
    inline namespace base64_literals {
        constexpr auto operator""_base64(char const* data, usize size) -> serialization::Base64<> {
            auto view = di::TransparentStringView { data, size };
            return parser::parse_unchecked<serialization::Base64<>>(view);
        }
    }
}
}

namespace di {
using serialization::Base64;

using Base64View = Base64<di::Span<byte const>>;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_BASE64_LITERALS)
using namespace di::literals::base64_literals;
#endif
