#pragma once

#include "di/container/string/string.h"
#include "di/types/prelude.h"
#include "di/vocab/span/prelude.h"

namespace ttx {
class Utf8StreamDecoder {
public:
    constexpr static auto replacement_character = U'\uFFFD';

    // Decode the incoming byte stream as UTF-8. This may buffer code
    // units if the input doesn't end on a code point boundary.
    //
    // Invalid UTF-8 sequences are replaced with replacement characters.
    auto decode(di::Span<byte const> input) -> di::String;

    // Flush any pending data. If there is any pending data, a single
    // replacement character will be output.
    auto flush() -> di::String;

private:
    constexpr static auto default_lower_bound = u8(0x80);
    constexpr static auto default_upper_bound = u8(0xBF);

    void decode_byte(di::String& output, byte input);
    void decode_first_byte(di::String& output, byte input);
    void output_code_point(di::String& output, c32 code_point);

    u8 m_pending_code_units { 0 };
    u32 m_pending_code_point { 0 };
    u8 m_lower_bound { default_lower_bound };
    u8 m_upper_bound { default_upper_bound };
};
}
