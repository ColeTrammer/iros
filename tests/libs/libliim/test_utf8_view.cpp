#include <liim/utf8_view.h>
#include <liim/vector.h>
#include <test/test.h>

TEST(utf8_view, basic) {
    auto view = Utf8View { "abcde" };
    EXPECT_EQ(view.size_in_bytes(), 5lu);
    EXPECT(!view.empty());

    auto unicode = Utf8View { "©" };
    EXPECT_EQ(unicode.size_in_bytes(), 2lu);
    EXPECT(!unicode.empty());
}

TEST(utf8_view, validate) {
    EXPECT(Utf8View { "abcde" }.validate());

    // clang-format off
    EXPECT(Utf8View { "⪶⑆⵻⼆⁅⥪◈♽⣭Ⲵ∱⚗⶜⭮⠲⥯⣿╓⺙⎊⏹⼈Ⲭ⯜⟜₇▉⫵⣽➲♕↔⢻∯⎬♦⧨★⩉ⵞ⁲’⊔⵪⇢❼⌠⁹❑⦎⭘⩳⓰⸙⇘➷♤⭕⵲⏂⭠⽮⎲⼷␋Ⱋ⽈Ⓦⱖ◻❄⩅Ⓩ ⡝⢾⻲ⵔ‟↙⍂⛕⤑╳⹩⯎ⶶ⇍ⳇ⃄⁈⫦⇜⟈⩃⒛⟸⸿⊣⹜" }.validate());
    // clang-format on

    EXPECT(!Utf8View { "\xC0\xAF" }.validate());
    EXPECT(!Utf8View { "\xE0\x0F\x80" }.validate());

    EXPECT(Utf8View { "€" }.validate());
    EXPECT(!Utf8View { "\xF0\x82\x82\xAC" }.validate());
}

auto to_vector = [](Utf8View view) -> Vector<uint32_t> {
    auto vector = Vector<uint32_t> {};
    for (auto code_point : view) {
        vector.add(code_point);
    }
    return vector;
};

TEST(utf8_view, iteration) {
    auto vec = to_vector(Utf8View { "$¢€𐍈" });
    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[0], 0x24U);
    EXPECT_EQ(vec[1], 0xA2U);
    EXPECT_EQ(vec[2], 0x20ACU);
    EXPECT_EQ(vec[3], 0x10348U);
}

// This tests that invalid UTF-8 sequences are replaced with
// U+FFFD in a uniform manner, as specified in https://encoding.spec.whatwg.org/#utf-8-decoder
TEST(utf8_view, replace_character_substitution_of_maximal_subparts) {
    auto vec = to_vector(Utf8View { "\xC0\xAF\xE0\x80\xBF\xF0\x81\x82\x41" });
    EXPECT_EQ(vec.size(), 9);
    EXPECT_EQ(vec[0], Utf8View::replacement_character);
    EXPECT_EQ(vec[1], Utf8View::replacement_character);
    EXPECT_EQ(vec[2], Utf8View::replacement_character);
    EXPECT_EQ(vec[3], Utf8View::replacement_character);
    EXPECT_EQ(vec[4], Utf8View::replacement_character);
    EXPECT_EQ(vec[5], Utf8View::replacement_character);
    EXPECT_EQ(vec[6], Utf8View::replacement_character);
    EXPECT_EQ(vec[7], Utf8View::replacement_character);
    EXPECT_EQ(vec[8], 0x41U);

    vec = to_vector(Utf8View { "\xED\xA0\x80\xED\xBF\xBF\xED\xAF\x41" });
    EXPECT_EQ(vec.size(), 9);
    EXPECT_EQ(vec[0], Utf8View::replacement_character);
    EXPECT_EQ(vec[1], Utf8View::replacement_character);
    EXPECT_EQ(vec[2], Utf8View::replacement_character);
    EXPECT_EQ(vec[3], Utf8View::replacement_character);
    EXPECT_EQ(vec[4], Utf8View::replacement_character);
    EXPECT_EQ(vec[5], Utf8View::replacement_character);
    EXPECT_EQ(vec[6], Utf8View::replacement_character);
    EXPECT_EQ(vec[7], Utf8View::replacement_character);
    EXPECT_EQ(vec[8], 0x41U);

    vec = to_vector(Utf8View { "\xF4\x91\x92\x93\xFF\x41\x80\xBF\x42" });
    EXPECT_EQ(vec.size(), 9);
    EXPECT_EQ(vec[0], Utf8View::replacement_character);
    EXPECT_EQ(vec[1], Utf8View::replacement_character);
    EXPECT_EQ(vec[2], Utf8View::replacement_character);
    EXPECT_EQ(vec[3], Utf8View::replacement_character);
    EXPECT_EQ(vec[4], Utf8View::replacement_character);
    EXPECT_EQ(vec[5], 0x41U);
    EXPECT_EQ(vec[6], Utf8View::replacement_character);
    EXPECT_EQ(vec[7], Utf8View::replacement_character);
    EXPECT_EQ(vec[8], 0x42U);

    vec = to_vector(Utf8View { "\xE1\x80\xE2\xF0\x91\x92\xF1\xBF\x41" });
    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec[0], Utf8View::replacement_character);
    EXPECT_EQ(vec[1], Utf8View::replacement_character);
    EXPECT_EQ(vec[2], Utf8View::replacement_character);
    EXPECT_EQ(vec[3], Utf8View::replacement_character);
    EXPECT_EQ(vec[4], 0x41U);
}
