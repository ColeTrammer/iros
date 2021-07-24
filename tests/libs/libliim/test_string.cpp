#include <liim/string.h>
#include <test/test.h>

TEST(string, basic_getters) {
    EXPECT(""s.empty());
    EXPECT("str"s[2] == 'r');
    EXPECT("str"s[1] == 't');
    EXPECT("str"s.size() == 3);
    EXPECT("abc"s.first() == 'a');
    EXPECT("abc"s.last() == 'c');
    EXPECT(""s[0] == '\0');
    EXPECT("abc"s[3] == '\0');
}

TEST(string, comparisons) {
    EXPECT(""s == ""s);
    EXPECT(""s != "abc"s);
    EXPECT("test"s == "test"s);
}

TEST(string, substrings) {
    EXPECT("hello"s.first(3) == "hel"s);
    EXPECT("hello"s.first(0) == ""s);
    EXPECT("hello"s.last(2) == "lo"s);
    EXPECT("hello"s.last(0) == ""s);
    EXPECT("hello"s.substring(1) == "ello"s);
    EXPECT("hello"s.substring(4) == "o"s);
    EXPECT("hello"s.substring(5) == ""s);
    EXPECT("hello"s.substring(2, 2) == "ll"s);
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
    EXPECT(haystack.index_of('h') == Maybe<size_t> { 0 });
    EXPECT(haystack.index_of('a') == Maybe<size_t> { 1 });
    EXPECT(haystack.index_of('k') == Maybe<size_t> { 7 });
    EXPECT(!haystack.index_of('z'));

    EXPECT(haystack.last_index_of('h') == Maybe<size_t> { 0 });
    EXPECT(haystack.last_index_of('a') == Maybe<size_t> { 5 });
    EXPECT(!haystack.last_index_of('z'));
}

TEST(string, split) {
    auto parts = "  testing   1  2 "s.split(' ');
    EXPECT(parts.size() == 3);
    EXPECT(parts[0] == "testing");
    EXPECT(parts[1] == "1");
    EXPECT(parts[2] == "2");

    EXPECT("   "s.split(' ').empty());
    EXPECT(""s.split(' ').empty());
}

TEST(string, join) {
    auto string = "this is fun"s;
    auto parts = string.split(' ');
    auto parts_view = string.split_view(' ');

    EXPECT(String::join(parts, '!') == "this!is!fun"s);
    EXPECT(String::join(parts_view, '!') == "this!is!fun"s);

    EXPECT(String::join(parts, '!', JoinPrependDelimiter::Yes) == "!this!is!fun"s);
    EXPECT(String::join(parts_view, '!', JoinPrependDelimiter::Yes) == "!this!is!fun"s);

    auto empty = Vector<String> {};
    EXPECT(String::join(empty, '!').empty());
    EXPECT(String::join(empty, '!', JoinPrependDelimiter::Yes) == "!"s);
}

TEST(string, repeat) {
    EXPECT(String::repeat('$', 4) == "$$$$"s);
    EXPECT(String::repeat('$', 0) == ""s);
}

TEST(string, case_conversions) {
    auto camelCase = "whatCase"s;
    auto PascalCase = "WhatCase"s;
    auto snake_case = "what_case"s;

    EXPECT(snake_case.to_title_case() == "WhatCase"s);
    EXPECT(snake_case == "WhatCase"s);

    EXPECT(PascalCase.to_lower_case() == "whatcase"s);
    EXPECT(PascalCase == "whatcase"s);

    EXPECT(camelCase.to_upper_case() == "WHATCASE"s);
    EXPECT(camelCase == "WHATCASE"s);

    EXPECT("aaa_bb_cc_"s.to_title_case() == "AaaBbCc");
}

TEST(string, mutators) {
    auto str = "abc"s;
    str += "dd"s;
    str += String('c');
    str += String("ee"sv);
    EXPECT(str == "abcddcee"s);

    str.clear();
    EXPECT(str.empty());

    str = "hi"s;
    EXPECT(str == "hi"s);

    str.insert('A', 1);
    EXPECT(str == "hAi"s);

    str.insert("rra"sv, 2);
    EXPECT(str == "hArrai"s);

    str.remove_index(4);
    EXPECT(str == "hArri"s);
}
