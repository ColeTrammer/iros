#include "di/container/view/range.h"
#include "di/vocab/array/to_array.h"
#include "di/vocab/span/as_bytes.h"
#include "dius/test/prelude.h"
#include "ttx/utf8_stream_decoder.h"

namespace utf8_stream_decoder {
static void basic() {
    // This string contains code points of all lengths (1,2,3,4);
    auto s = u8"$¢€𐍈"_sv;
    auto as_bytes = di::as_bytes(s.span());

    // Try all possible segmentations of the byte string, and make sure they
    // decode to the correct output string.
    auto decoder = ttx::Utf8StreamDecoder {};
    for (auto i : di::range(s.size_bytes() + 1)) {
        auto s1 = decoder.decode(as_bytes.subspan(0, i).value_or({}));
        auto s2 = decoder.decode(as_bytes.subspan(i).value_or({}));

        s1.append(di::move(s2));
        ASSERT_EQ(s1, s);
    }
}

static void errors() {
    // This tests that invalid UTF-8 sequences are replaced with
    // U+FFFD in a uniform manner, as specified in the Unicode core specification:
    // Substitution of Maximumal Subparts: https://www.unicode.org/versions/Unicode16.0.0/core-spec/chapter-3/#G66453

    auto decoder = ttx::Utf8StreamDecoder {};

    struct Case {
        c8 const* input { nullptr };
        di::Vector<c32> expected;
    };

    constexpr auto replacement_character = ttx::Utf8StreamDecoder::replacement_character;
    auto cases = di::to_array<Case>({
        { u8"\xC0\xAF\xE0\x80\xBF\xF0\x81\x82\x41",
          { replacement_character, replacement_character, replacement_character, replacement_character,
            replacement_character, replacement_character, replacement_character, replacement_character, 0x41 } },
        { u8"\xED\xA0\x80\xED\xBF\xBF\xED\xAF\x41",
          { replacement_character, replacement_character, replacement_character, replacement_character,
            replacement_character, replacement_character, replacement_character, replacement_character, 0x41 } },
        { u8"\xF4\x91\x92\x93\xFF\x41\x80\xBF\x42",
          { replacement_character, replacement_character, replacement_character, replacement_character,
            replacement_character, 0x41, replacement_character, replacement_character, 0x42 } },
        { u8"\xE1\x80\xE2\xF0\x91\x92\xF1\xBF\x41",
          { replacement_character, replacement_character, replacement_character, replacement_character, 0x41 } },
    });

    for (auto const& [input, expected] : cases) {
        auto input_span = di::Span(input, input + di::distance(di::ZC8CString(input)));
        auto actual = decoder.decode(di::as_bytes(input_span));
        actual.append(decoder.flush());

        ASSERT_EQ(actual, expected | di::to<di::String>());
    }
}

TEST(utf8_stream_decoder, basic)
TEST(utf8_stream_decoder, errors)
}
