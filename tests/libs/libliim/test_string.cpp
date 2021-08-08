#include <liim/string.h>
#include <test/test.h>

TEST(string, basic_getters) {
    EXPECT(""s.empty());
    EXPECT_EQ("str"s[2], 'r');
    EXPECT_EQ("str"s[1], 't');
    EXPECT_EQ("str"s.size(), 3lu);
    EXPECT_EQ("abc"s.first(), 'a');
    EXPECT_EQ("abc"s.last(), 'c');
    EXPECT_EQ(""s[0], '\0');
    EXPECT_EQ("abc"s[3], '\0');
}

TEST(string, comparisons) {
    EXPECT_EQ(""s, ""s);
    EXPECT_NOT_EQ(""s, "abc"s);
    EXPECT_EQ("test"s, "test"s);
}

TEST(string, substrings) {
    EXPECT_EQ("hello"s.first(3), "hel"s);
    EXPECT_EQ("hello"s.first(0), ""s);
    EXPECT_EQ("hello"s.last(2), "lo"s);
    EXPECT_EQ("hello"s.last(0), ""s);
    EXPECT_EQ("hello"s.substring(1), "ello"s);
    EXPECT_EQ("hello"s.substring(4), "o"s);
    EXPECT_EQ("hello"s.substring(5), ""s);
    EXPECT_EQ("hello"s.substring(2, 2), "ll"s);
}

TEST(string, contains) {
    auto haystack = "long string"s;
    EXPECT(haystack.starts_with("long"));
    EXPECT(haystack.starts_with("long s"));
    EXPECT(!haystack.starts_with("ll"));
    EXPECT(haystack.starts_with(""));

    EXPECT(haystack.ends_with("ing"));
    EXPECT(haystack.ends_with("string"));
    EXPECT(!haystack.ends_with("gg"));
    EXPECT(haystack.ends_with(""));
}

TEST(string, find) {
    auto haystack = "haystack"s;
    EXPECT_EQ(haystack.index_of('h'), Maybe<size_t> { 0 });
    EXPECT_EQ(haystack.index_of('a'), Maybe<size_t> { 1 });
    EXPECT_EQ(haystack.index_of('k'), Maybe<size_t> { 7 });
    EXPECT(!haystack.index_of('z'));

    EXPECT_EQ(haystack.last_index_of('h'), Maybe<size_t> { 0 });
    EXPECT_EQ(haystack.last_index_of('a'), Maybe<size_t> { 5 });
    EXPECT(!haystack.last_index_of('z'));
}

TEST(string, split) {
    auto parts = "  testing   1  2 "s.split(' ');
    EXPECT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "testing");
    EXPECT_EQ(parts[1], "1");
    EXPECT_EQ(parts[2], "2");

    EXPECT("   "s.split(' ').empty());
    EXPECT(""s.split(' ').empty());
}

TEST(string, join) {
    auto string = "this is fun"s;
    auto parts = string.split(' ');
    auto parts_view = string.split_view(' ');

    EXPECT_EQ(String::join(parts, '!'), "this!is!fun"s);
    EXPECT_EQ(String::join(parts_view, '!'), "this!is!fun"s);

    EXPECT_EQ(String::join(parts, '!', JoinPrependDelimiter::Yes), "!this!is!fun"s);
    EXPECT_EQ(String::join(parts_view, '!', JoinPrependDelimiter::Yes), "!this!is!fun"s);

    auto empty = Vector<String> {};
    EXPECT(String::join(empty, '!').empty());
    EXPECT_EQ(String::join(empty, '!', JoinPrependDelimiter::Yes), "!"s);
}

TEST(string, repeat) {
    EXPECT_EQ(String::repeat('$', 4), "$$$$"s);
    EXPECT_EQ(String::repeat('$', 0), ""s);
}

TEST(string, case_conversions) {
    auto camelCase = "whatCase"s;
    auto PascalCase = "WhatCase"s;
    auto snake_case = "what_case"s;

    EXPECT_EQ(snake_case.to_title_case(), "WhatCase"s);
    EXPECT_EQ(snake_case, "WhatCase"s);

    EXPECT_EQ(PascalCase.to_lower_case(), "whatcase"s);
    EXPECT_EQ(PascalCase, "whatcase"s);

    EXPECT_EQ(camelCase.to_upper_case(), "WHATCASE"s);
    EXPECT_EQ(camelCase, "WHATCASE"s);

    EXPECT_EQ("aaa_bb_cc_"s.to_title_case(), "AaaBbCc");
}

TEST(string, mutators) {
    auto str = "abc"s;
    str += "dd"s;
    str += String('c');
    str += String("ee"sv);
    EXPECT_EQ(str, "abcddcee"s);

    str.clear();
    EXPECT(str.empty());

    str = "hi"s;
    EXPECT_EQ(str, "hi"s);

    str.insert('A', 1);
    EXPECT_EQ(str, "hAi"s);

    str.insert("rra"sv, 2);
    EXPECT_EQ(str, "hArrai"s);

    str.remove_index(4);
    EXPECT_EQ(str, "hArri"s);
}
