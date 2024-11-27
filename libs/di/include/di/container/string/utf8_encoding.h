#pragma once

#include <di/container/iterator/iterator_base.h>
#include <di/container/string/encoding.h>
#include <di/container/vector/static_vector.h>
#include <di/container/view/range.h>
#include <di/vocab/span/prelude.h>

namespace di::container::string {
// NOTE: see https://www.unicode.org/versions/Unicode14.0.0/UnicodeStandard-14.0.pdf for details on the UTF-8 encoding.
//       In particular, section 3.9, table 3-6, and table 3-7.
namespace utf8 {
    constexpr static auto is_start_of_one_byte_sequence(c8 byte) -> bool {
        return byte <= 0x7F;
    }

    constexpr static auto is_start_of_two_byte_sequence(c8 byte) -> bool {
        return byte >= 0xC2 && byte <= 0xDF;
    }

    constexpr static auto is_start_of_three_byte_sequence(c8 byte) -> bool {
        return byte >= 0xE0 && byte <= 0xEF;
    }

    constexpr static auto is_start_of_four_byte_sequence(c8 byte) -> bool {
        return byte >= 0xF0 && byte <= 0xF4;
    }

    constexpr static auto is_start_of_multi_byte_sequence(c8 byte) -> bool {
        return is_start_of_two_byte_sequence(byte) || is_start_of_three_byte_sequence(byte) ||
               is_start_of_four_byte_sequence(byte);
    }

    constexpr static auto is_valid_first_byte(c8 byte) -> bool {
        return is_start_of_one_byte_sequence(byte) || is_start_of_multi_byte_sequence(byte);
    }

    constexpr static auto is_valid_second_byte(c8 first_byte, c8 second_byte) -> bool {
        switch (first_byte) {
            case 0xE0:
                return second_byte >= 0xA0 && second_byte <= 0xBF;
            case 0xED:
                return second_byte >= 0x80 && second_byte <= 0x9F;
            case 0xF0:
                return second_byte >= 0x90 && second_byte <= 0xBF;
            case 0xF4:
                return second_byte >= 0x80 && second_byte <= 0x8F;
            default:
                return second_byte >= 0x80 && second_byte <= 0xBF;
        }
    }

    constexpr static auto is_valid_third_byte([[maybe_unused]] c8 first_byte, c8 third_byte) -> bool {
        return third_byte >= 0x80 && third_byte <= 0xBF;
    }

    constexpr static auto is_valid_fourth_byte([[maybe_unused]] c8 first_byte, c8 fourth_byte) -> bool {
        return fourth_byte >= 0x80 && fourth_byte <= 0xBF;
    }

    constexpr static auto byte_sequence_length(c8 first_byte) -> u8 {
        return is_start_of_one_byte_sequence(first_byte)     ? 1
               : is_start_of_two_byte_sequence(first_byte)   ? 2
               : is_start_of_three_byte_sequence(first_byte) ? 3
                                                             : 4;
    }

    class Utf8Iterator : public IteratorBase<Utf8Iterator, BidirectionalIteratorTag, c32, ssize_t> {
    public:
        Utf8Iterator() = default;
        constexpr explicit Utf8Iterator(c8 const* data) : m_data(data) {}

        constexpr auto operator*() const -> c32 {
            auto length = byte_sequence_length(*m_data);
            auto first_byte_mask = 0b11111111 >> length;
            auto result = static_cast<c32>(*m_data & first_byte_mask);
            for (auto i : view::range(1u, length)) {
                result <<= 6;
                result |= m_data[i] & 0b00111111;
            }
            return result;
        }

        constexpr void advance_one() { m_data += byte_sequence_length(*m_data); }
        constexpr void back_one() {
            do {
                --m_data;
            } while (!is_valid_first_byte(*m_data));
        }

        constexpr auto data() const -> c8 const* { return m_data; }

        constexpr explicit operator c8 const*() const { return data(); }

    private:
        constexpr friend auto operator==(Utf8Iterator const& a, Utf8Iterator const& b) -> bool {
            return a.data() == b.data();
        }
        constexpr friend auto operator<=>(Utf8Iterator const& a, Utf8Iterator const& b) {
            return a.data() <=> b.data();
        }

        c8 const* m_data { nullptr };
    };
}

class Utf8Encoding {
public:
    using CodeUnit = c8;
    using CodePoint = c32;
    using Iterator = utf8::Utf8Iterator;

private:
    template<typename = void>
    constexpr friend auto tag_invoke(types::Tag<encoding::validate>, Utf8Encoding const&, Span<c8 const> data) -> bool {
        size_t i = 0;
        while (i < data.size()) {
            auto first_byte = data.data()[i];
            if (!utf8::is_valid_first_byte(first_byte)) {
                return false;
            }
            auto length = utf8::byte_sequence_length(first_byte);
            if (i + length > data.size()) {
                return false;
            }
            switch (length) {
                case 4:
                    if (!utf8::is_valid_fourth_byte(first_byte, data.data()[i + 3])) {
                        return false;
                    }
                    [[fallthrough]];
                case 3:
                    if (!utf8::is_valid_third_byte(first_byte, data.data()[i + 2])) {
                        return false;
                    }
                    [[fallthrough]];
                case 2:
                    if (!utf8::is_valid_second_byte(first_byte, data.data()[i + 1])) {
                        return false;
                    }
            }
            i += length;
        }
        return true;
    }

    constexpr friend auto tag_invoke(types::Tag<encoding::valid_byte_offset>, Utf8Encoding const&, Span<c8 const> data,
                                     size_t offset) -> bool {
        // NOTE: this function can assume the underlying c8 data is valid UTF-8.
        if (offset >= data.size()) {
            return offset == data.size();
        }
        return utf8::is_valid_first_byte(data[offset]);
    }

    constexpr friend auto tag_invoke(types::Tag<encoding::convert_to_code_units>, Utf8Encoding const&, c32 code_point) {
        auto result = container::StaticVector<c8, meta::Constexpr<4zu>> {};
        auto code_point_value = static_cast<u32>(code_point);
        if (code_point_value <= 0x7F) {
            (void) result.resize(1);
            result[0] = code_point_value;
        } else if (code_point_value <= 0x7FF) {
            (void) result.resize(2);
            result[0] = 0b11000000 | (code_point_value >> 6);
            result[1] = 0b10000000 | (code_point_value & 0x3F);
        } else if (code_point_value <= 0xFFFF) {
            (void) result.resize(3);
            result[0] = 0b11100000 | (code_point_value >> 12);
            result[1] = 0b10000000 | ((code_point_value >> 6) & 0x3F);
            result[2] = 0b10000000 | (code_point_value & 0x3F);
        } else {
            (void) result.resize(4);
            result[0] = 0b11110000 | (code_point_value >> 18);
            result[1] = 0b10000000 | ((code_point_value >> 12) & 0x3F);
            result[2] = 0b10000000 | ((code_point_value >> 6) & 0x3F);
            result[3] = 0b10000000 | (code_point_value & 0x3F);
        }
        return result;
    }
};
}
