#include "ttx/utf8_stream_decoder.h"

#include "di/function/between_inclusive.h"
#include "di/parser/integral.h"
#include "di/types/byte.h"

namespace ttx {

auto Utf8StreamDecoder::decode(di::Span<byte const> input) -> di::String {
    auto result = ""_s;
    for (auto byte : input) {
        decode_byte(result, byte);
    }
    return result;
}

auto Utf8StreamDecoder::flush() -> di::String {
    auto result = ""_s;
    if (m_pending_code_units > 0) {
        output_code_point(result, replacement_character);
    }
    return result;
}

void Utf8StreamDecoder::decode_byte(di::String& output, byte input) {
    if (m_pending_code_units == 0) {
        decode_first_byte(output, input);
        return;
    }

    auto input_u8 = di::to_integer<u8>(input);
    if (!di::between_inclusive(input_u8, m_lower_bound, m_upper_bound)) {
        output_code_point(output, replacement_character);
        decode_first_byte(output, input);
        return;
    }

    m_lower_bound = default_lower_bound;
    m_upper_bound = default_upper_bound;
    m_pending_code_point <<= 6;
    m_pending_code_point |= di::to_integer<u32>(input & 0b0011'1111_b);
    if (--m_pending_code_units == 0) {
        output_code_point(output, m_pending_code_point);
    }
}

void Utf8StreamDecoder::decode_first_byte(di::String& output, byte input) {
    // Valid ranges come from the Unicode core specification, table 3-7.
    //   https://www.unicode.org/versions/Unicode16.0.0/core-spec/chapter-3/#G27506
    auto input_u8 = di::to_integer<u8>(input);
    if (di::between_inclusive(input_u8, 0x00, 0x7F)) {
        output_code_point(output, di::to_integer<u32>(input));
    } else if (di::between_inclusive(input_u8, 0xC2, 0xDF)) {
        m_pending_code_units = 1;
        m_pending_code_point = di::to_integer<u32>(input & 0x1F_b);
    } else if (di::between_inclusive(input_u8, 0xE0, 0xEF)) {
        m_pending_code_units = 2;
        if (input == 0xE0_b) {
            m_lower_bound = 0xA0;
        } else if (input == 0xED_b) {
            m_upper_bound = 0x9F;
        }
        m_pending_code_point = di::to_integer<u32>(input & 0x0F_b);
    } else if (di::between_inclusive(input_u8, 0xF0, 0xF4)) {
        m_pending_code_units = 3;
        if (input == 0xF0_b) {
            m_lower_bound = 0x90;
        } else if (input == 0xF4_b) {
            m_upper_bound = 0x8F;
        }
        m_pending_code_point = di::to_integer<u32>(input & 0x07_b);
    } else {
        output_code_point(output, replacement_character);
    }
}

void Utf8StreamDecoder::output_code_point(di::String& output, c32 code_point) {
    output.push_back(code_point);

    // Reset state.
    *this = {};
}
}
