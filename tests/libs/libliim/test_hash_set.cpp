#include <liim/hash/hashable.h>
#include <liim/hash/hasher.h>
#include <liim/hash/set.h>
#include <test/test.h>

namespace LIIM::Hash {
template<>
struct HashFunction<int> {
    static constexpr void hash(Hasher&, int) {}
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
    EXPECT(!set.contains(7));

    auto it = set.begin();
    ++it;
    ++it;
    set.erase(set.begin(), it);
    EXPECT_EQ(set.size(), 2u);

    set.erase(set.begin());
    EXPECT_EQ(set.size(), 1u);
}

constexpr void comparison() {
    auto a = LIIM::Hash::Set { 2, 3, 4 };
    auto b = a;
    EXPECT_EQ(a, b);
    a.insert(5);
    EXPECT_NOT_EQ(a, b);
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
TEST(hash_set, format) {
    format();
}
