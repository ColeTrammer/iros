#include <liim/container/algorithm/reverse.h>
#include <liim/container/array.h>
#include <liim/container/new_vector.h>
#include <liim/container/string_view.h>
#include <test/test.h>

constexpr LIIM::Container::AsciiStringView operator""_av(char const* data, size_t size) {
    return { LIIM::Container::Strings::AssumeProperlyEncoded {}, data, size };
}

constexpr LIIM::Container::StringView operator""_sv(char const* data, size_t size) {
    assert(LIIM::Container::Strings::Utf8Encoding::is_valid({ data, size }));
    return { LIIM::Container::Strings::AssumeProperlyEncoded {}, data, size };
}

constexpr void ascii() {
    auto x = "hello"_av;
    EXPECT(x.starts_with("hel"_av));
    EXPECT(x.ends_with("llo"_av));

    EXPECT("hello"_av <=> "hel"_av > 0);
    EXPECT("hello"_av.size_in_code_points(), 5);

    EXPECT(""_av.empty());
    EXPECT_EQ("xyz"_av.front(), 'x');
    EXPECT_EQ("xyz"_av.back(), 'z');
    EXPECT(!""_av.front());
    EXPECT(!""_av.back());

    EXPECT(x.substr(*x.iterator_at_offset(1), *x.iterator_at_offset(4)) == "ell"_av);

    for (auto c : x) {
        EXPECT(c > 32);
    }

    EXPECT("hello"_av == x);
}

constexpr void do_utf8_test(LIIM::Container::StringView view, NewVector<char32_t> const& desired) {
    // Check the iteration produces the same results forwards and backwards.
    auto forwards = collect_vector(view);
    auto backwards = collect_vector(reversed(view));
    Alg::reverse(backwards);

    EXPECT_EQ(forwards, backwards);
    EXPECT_EQ(forwards, desired);
    EXPECT_EQ(backwards, desired);
};

constexpr void utf8() {
    auto x = "$¢€𐍈"_sv;

    EXPECT(x.starts_with("$"_sv));
    EXPECT(x.ends_with("€𐍈"_sv));
    EXPECT(!x.ends_with("¢"_sv));

    EXPECT_EQ(x.front(), U'$');
    EXPECT_EQ(x.back(), U'𐍈');

    EXPECT_EQ(x.size_in_bytes(), 10u);
    EXPECT_EQ(x.size_in_code_points(), 4u);

    EXPECT("¢€"_sv == x.substr(*x.iterator_at_offset(1), *x.iterator_at_offset(6)));
    EXPECT(!x.iterator_at_offset(2));
    EXPECT(!x.iterator_at_offset(4));

    EXPECT(x.substr(x.begin(), *x.iterator_at_offset(1)) == "$"_sv);
    EXPECT(x.substr(*x.iterator_at_offset(1)) == "¢€𐍈"_sv);

    auto vec = collect_vector("$¢€𐍈"_sv);
    do_utf8_test("$¢€𐍈"_sv, collect_vector(Array {
                                U'\x24',
                                U'\xA2',
                                U'\x20AC',
                                U'\x10348',
                            }));
}

// This tests that invalid UTF-8 sequences are replaced with U+FFFD in a uniform manner, as specified in
// https://encoding.spec.whatwg.org/#utf-8-decoder
constexpr void utf8_replace_character_substitution_of_maximal_subparts() {
    constexpr char32_t replacement_character = LIIM::Container::Strings::Utf8::replacement_character;

    using SV = LIIM::Container::StringView;

    // From the Unicode Standard Table 3-8: U+FFFD for Non-Shortest Form Sequences
    do_utf8_test(SV::create_unchecked_from_null_terminated_string("\xC0\xAF\xE0\x80\xBF\xF0\x81\x82\x41"), collect_vector(Array {
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               U'\x41',
                                                                                                           }));

    // From the Unicode Standard Table 3-9: U+FFFD for Ill-Formed Sequences for Surrogates
    do_utf8_test(SV::create_unchecked_from_null_terminated_string("\xED\xA0\x80\xED\xBF\xBF\xED\xAF\x41"), collect_vector(Array {
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               U'\x41',
                                                                                                           }));

    // From the Unicode Standard Table 3-10: U+FFFD for Other Ill-Formed Sequences
    do_utf8_test(SV::create_unchecked_from_null_terminated_string("\xF4\x91\x92\x93\xFF\x41\x80\xBF\x42"), collect_vector(Array {
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               U'\x41',
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               U'\x42',
                                                                                                           }));

    // From the Unicode Standard Table 3-11: U+FFFD for Truncated Sequences
    do_utf8_test(SV::create_unchecked_from_null_terminated_string("\xE1\x80\xE2\xF0\x91\x92\xF1\xBF\x41"), collect_vector(Array {
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               replacement_character,
                                                                                                               U'\x41',
                                                                                                           }));
}

TEST_CONSTEXPR(strings, ascii, ascii)
TEST_CONSTEXPR(strings, utf8, utf8)
TEST_CONSTEXPR(strings, utf8_replace_character_substitution_of_maximal_subparts, utf8_replace_character_substitution_of_maximal_subparts)
