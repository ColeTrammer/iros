#include <liim/string_view.h>
#include <test/test.h>

TEST(string_view, basic_getters) {
    EXPECT(""sv.empty());
    EXPECT_EQ("abc"sv.size(), 3lu);
    EXPECT_EQ("liim"sv.char_at(3), 'm');
    EXPECT_EQ("liim"sv.first(), 'l');
    EXPECT_EQ("liim"sv.last(), 'm');
}

TEST(string_view, comparisons) {
    EXPECT_EQ(""sv, ""sv);
    EXPECT_NOT_EQ("A"sv, ""sv);
    EXPECT_EQ("string_view"sv, "string_view"sv);
    EXPECT_NOT_EQ("string_view"sv, "string"sv);
}

TEST(string_view, substrings) {
    auto test = "test string literal"sv;
    EXPECT_EQ(test.first(4), "test"sv);
    EXPECT_EQ(test.first(0), ""sv);
    EXPECT_EQ(test.substring(5), "string literal"sv);
    EXPECT_EQ(test.substring(test.size()), ""sv);
    EXPECT_EQ(test.last(7), "literal"sv);
    EXPECT_EQ(test.substring(5, 6), "string"sv);
}

TEST(string_view, contains) {
    auto haystack = "sufficently long string"sv;
    EXPECT(haystack.starts_with("sufficently"sv));
    EXPECT(!haystack.starts_with("an even longer long string"sv));
    EXPECT(!haystack.starts_with("string"sv));

    EXPECT(haystack.ends_with("string"sv));
    EXPECT(!haystack.ends_with("an even longer long string"sv));
    EXPECT(!haystack.ends_with("sufficently"sv));
}

TEST(string_view, find) {
    auto haystack = "interesting"sv;
    EXPECT(!haystack.index_of('z'));
    EXPECT_EQ(haystack.index_of('i'), Maybe<size_t> { 0 });
    EXPECT_EQ(haystack.index_of('t'), Maybe<size_t> { 2 });

    EXPECT(!haystack.last_index_of('z'));
    EXPECT_EQ(haystack.last_index_of('t'), Maybe<size_t> { 7 });
    EXPECT_EQ(haystack.last_index_of('g'), Maybe<size_t> { 10 });
    EXPECT_EQ("abc"sv.last_index_of('a'), Maybe<size_t> { 0 });
}

TEST(string_view, split) {
    auto view = "  a series  of   words "sv;
    auto parts = view.split(' ');
    EXPECT_EQ(parts.size(), 4);
    EXPECT_EQ(parts[0], "a"sv);
    EXPECT_EQ(parts[1], "series"sv);
    EXPECT_EQ(parts[2], "of"sv);
    EXPECT_EQ(parts[3], "words"sv);

    EXPECT("   "sv.split(' ').empty());
    EXPECT(""sv.split(' ').empty());
}
