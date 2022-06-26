#include <liim/container/hash/hashable.h>
#include <liim/container/hash/hasher.h>
#include <liim/container/hash_set.h>
#include <liim/string.h>
#include <test/test.h>

constexpr void basic() {
    auto set = LIIM::Container::HashSet<int> {};
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

    set = LIIM::Container::make_hash_set({ 1, 2, 3 });
    EXPECT_EQ(set.size(), 3u);
    EXPECT(set.contains(2));
    set.insert({ 3, 4, 5 });
    EXPECT_EQ(set.size(), 5u);
    EXPECT(set.contains(5));
}

constexpr void containers() {
    auto set = LIIM::Container::HashSet<int> {};
    insert(set, set.end(), range(50, 55));
    for (auto i : range(50, 55)) {
        set.contains(i);
    }
    for (auto i : range(50)) {
        EXPECT(!set.contains(i));
    }
}

constexpr void erase() {
    auto set = make_hash_set({ 5, 6, 7, 8, 9 });
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
    auto a = make_hash_set({ 2, 3, 4 });
    auto b = a.clone();
    EXPECT_EQ(a, b);
    a.insert(5);
    EXPECT_NOT_EQ(a, b);
}

static void transparent() {
    auto a = make_hash_set({ "abc"s, "def"s });
    EXPECT_EQ(*a.at("abc"sv), "abc"s);
    EXPECT(!a.at("aaa"sv));
    EXPECT_EQ(*a.insert("abc"sv), "abc");
    EXPECT(!a.insert("xyz"sv));
    EXPECT_EQ(*a.at("xyz"sv), "xyz"sv);
    EXPECT_EQ(a.erase("xyz"sv), "xyz"s);
    EXPECT(!a.erase("xyz"sv));

    auto b = make_hash_set({ "xyz"sv, "sv"sv });
    EXPECT_EQ(*b.at("xyz"s), "xyz"sv);
    EXPECT_EQ(*b.at("xyz"), "xyz"sv);
    EXPECT(!b.at("xyzz"));

    auto c = make_hash_set({ "abc", "def" });
    EXPECT_EQ(*c.at("abc"sv), "abc"sv);
    EXPECT(!b.at("xyzz"s));
}

static void format() {
    auto a = make_hash_set({ 2, 3, 4 });
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
