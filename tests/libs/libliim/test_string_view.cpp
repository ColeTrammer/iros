#include <liim/string_view.h>
#include <test/test.h>

TEST(string_view, basic_getters) {
    EXPECT(""sv.empty());
    EXPECT("abc"sv.size() == 3);
    EXPECT("liim"sv.char_at(3) == 'm');
    EXPECT("liim"sv.first() == 'l');
    EXPECT("liim"sv.last() == 'm');
}

TEST(string_view, comparisons) {
    EXPECT(""sv == ""sv);
    EXPECT("A"sv != ""sv);
    EXPECT("string_view"sv == "string_view"sv);
    EXPECT("string_view"sv != "string"sv);
}

TEST(string_view, substrings) {
    auto test = "test string literal"sv;
    EXPECT(test.first(4) == "test"sv);
    EXPECT(test.first(0) == ""sv);
    EXPECT(test.after(5) == "string literal"sv);
    EXPECT(test.after(test.size()) == ""sv);
    EXPECT(test.last(7) == "literal"sv);
    EXPECT(test.substring(5, 6) == "string"sv);
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
    EXPECT(haystack.index_of('i') == Maybe<size_t> { 0 });
    EXPECT(haystack.index_of('t') == Maybe<size_t> { 2 });

    EXPECT(!haystack.last_index_of('z'));
    EXPECT(haystack.last_index_of('t') == Maybe<size_t> { 7 });
    EXPECT(haystack.last_index_of('g') == Maybe<size_t> { 10 });
    EXPECT("abc"sv.last_index_of('a') == Maybe<size_t> { 0 });
}

TEST(string_view, split) {
    auto view = "  a series  of   words "sv;
    auto parts = view.split(' ');
    EXPECT(parts.size() == 4);
    EXPECT(parts[0] == "a"sv);
    EXPECT(parts[1] == "series"sv);
    EXPECT(parts[2] == "of"sv);
    EXPECT(parts[3] == "words"sv);

    EXPECT("   "sv.split(' ').empty());
    EXPECT(""sv.split(' ').empty());
}
