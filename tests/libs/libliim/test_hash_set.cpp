#include <liim/hash/hashable.h>
#include <liim/hash/hasher.h>
#include <liim/hash/set.h>
#include <liim/string.h>
#include <test/test.h>

namespace LIIM::Hash {
template<>
struct HashFunction<int> {
    static constexpr void hash(Hasher&, int) {}
};

template<>
struct HashFunction<String> {
    static constexpr void hash(Hasher&, const String&) {}

    using Matches = Tuple<StringView, const char*>;
};

template<>
struct HashFunction<StringView> {
    static constexpr void hash(Hasher&, StringView) {}

    using Matches = Tuple<String, const char*>;
};

template<>
struct HashFunction<const char*> {
    static constexpr void hash(Hasher&, const char*) {}

    using Matches = Tuple<StringView, String>;
};
}

constexpr void basic() {
    auto set = LIIM::Hash::Set<int> {};
    for (auto i : range(50, 55)) {
        set.insert(i);
    }
    for (auto i : range(50, 55)) {
        set.contains(i);
    }
    for (auto i : range(50)) {
        EXPECT(!set.contains(i));
    }
    set.clear();
    EXPECT_EQ(set.size(), 0u);

    set = { 1, 2, 3 };
    EXPECT_EQ(set.size(), 3u);
    EXPECT(set.contains(2));
    set.insert({ 3, 4, 5 });
    EXPECT_EQ(set.size(), 5u);
    EXPECT(set.contains(5));
}

constexpr void containers() {
    auto set = LIIM::Hash::Set<int> {};
    set.insert(range(50, 55));
    for (auto i : range(50, 55)) {
        set.contains(i);
    }
    for (auto i : range(50)) {
        EXPECT(!set.contains(i));
    }
}

constexpr void erase() {
    auto set = LIIM::Hash::Set { 5, 6, 7, 8, 9 };
    EXPECT_EQ(set.size(), 5u);

    EXPECT_EQ(set.erase(7), 7);
    EXPECT_EQ(set.size(), 4u);
    EXPECT_EQ(set.erase(set.find(9)), 9);
    EXPECT_EQ(set.size(), 3u);
    EXPECT(!set.contains(7));

    auto it = set.begin();
    ++it;
    ++it;
    set.erase(set.begin(), it);
    EXPECT_EQ(set.size(), 1u);

    set.erase(set.begin());
    EXPECT_EQ(set.size(), 0u);
}

constexpr void comparison() {
    auto a = LIIM::Hash::Set { 2, 3, 4 };
    auto b = a;
    EXPECT_EQ(a, b);
    a.insert(5);
    EXPECT_NOT_EQ(a, b);
}

static void transparent() {
    static_assert(LIIM::Hash::Detail::CanInsert<StringView, String>);

    auto a = LIIM::Hash::Set<String> { "abc"s, "def"s };
    EXPECT_EQ(*a.at("abc"sv), "abc"s);
    EXPECT(!a.at("aaa"sv));
    EXPECT_EQ(*a.insert("abc"sv), "abc");
    EXPECT(!a.insert("xyz"sv));
    EXPECT_EQ(*a.at("xyz"sv), "xyz"sv);
    EXPECT_EQ(a.erase("xyz"sv), "xyz"s);
    EXPECT(!a.erase("xyz"sv));

    auto b = LIIM::Hash::Set { "xyz"sv, "sv"sv };
    EXPECT_EQ(*b.at("xyz"s), "xyz"sv);
    EXPECT_EQ(*b.at("xyz"), "xyz"sv);
    EXPECT(!b.at("xyzz"));

    auto c = LIIM::Hash::Set { "abc", "def" };
    EXPECT_EQ(*c.at("abc"sv), "abc"sv);
    EXPECT(!b.at("xyzz"s));
}

static void format() {
    auto a = LIIM::Hash::Set { 2, 3, 4 };
    EXPECT_EQ(to_string(a), "{ 2, 3, 4 }");
    EXPECT_EQ(to_string(a, "{:#x}"), "{ 0x2, 0x3, 0x4 }");
}

TEST_CONSTEXPR(hash_set, basic, basic)
TEST_CONSTEXPR(hash_set, containers, containers)
TEST_CONSTEXPR(hash_set, erase, erase)
TEST_CONSTEXPR(hash_set, comparison, comparison)
TEST(hash_set, transparent) {
    transparent();
}
TEST(hash_set, format) {
    format();
}
