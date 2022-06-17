#include <liim/container/hash_map.h>
#include <liim/container/hash_set.h>
#include <liim/container/new_vector.h>
#include <liim/string.h>
#include <test/test.h>

namespace LIIM::Container::Hash {
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

using namespace LIIM::Container;

constexpr void basic() {
    auto map = LIIM::Container::HashMap<int, int> {};
    EXPECT(!map.contains(0));

    EXPECT_EQ(map[2], 0);
    map[3] = 4;
    EXPECT(map.contains(3));
    EXPECT_EQ(*map.at(3), 4);

    EXPECT(!map.try_emplace(6, 6));
    EXPECT_EQ(*map.at(6), 6);

    EXPECT_EQ(map.insert_or_assign(3, 5), 4);
    EXPECT_EQ(*map.at(3), 5);

    map.insert(make_pair(42, 400));
    EXPECT(map.contains(42));
}

constexpr void containers() {
    auto items = make_vector<Pair<int, int>>({ { 1, 1 }, { 2, 4 }, { 3, 9 } });
    auto map = collect_hash_map(move(items));
    EXPECT_EQ(*map.at(1), 1);

    insert(map, map.begin(), zip(std::initializer_list<int> { 4, 5, 6 }, std::initializer_list<int> { 16, 25, 36 }));

    EXPECT_EQ(map.size(), 6u);
    for (auto& [k, v] : map) {
        EXPECT_EQ(k * k, v);
    }
}

constexpr void erase() {
    auto items = collect_hash_map(zip(range(6), range(6, 12)));
    EXPECT_EQ(items.erase(3), 9);
    EXPECT(!items.erase(10));

    EXPECT_EQ(items.erase(items.find(1)), 7);
}

constexpr void comparison() {
    auto a = collect_hash_map(zip(range(4), range(4, 8)));
    auto b = a.clone();
    EXPECT_EQ(a, b);

    a.erase(0);
    EXPECT_NOT_EQ(a, b);
}

constexpr void subcontainers() {
    auto keys = collect_hash_set(range(4));
    auto values = collect_hash_set(range(4, 8));
    auto a = collect_hash_map(zip(keys, values));
    EXPECT_EQ(collect_hash_set(a.keys()), move(keys));
    EXPECT_EQ(collect_hash_set(a.values()), move(values));
}

static void transparent() {
    {
        auto x = collect<LIIM::Container::HashMap<String, String>>(
            zip(std::initializer_list { "hello"sv, "world"sv }, std::initializer_list { "xxx"sv, "yyy"sv }));

        EXPECT_EQ(*x.at("hello"sv), "xxx");
        EXPECT_EQ(x["world"sv], "yyy");
        EXPECT_EQ(x["friends"sv], "");
        EXPECT_EQ(x.size(), 3u);

        EXPECT(x.contains("hello"sv));
        EXPECT_EQ(*x.try_emplace("hello"sv, "zzz"sv), "xxx");
        EXPECT_EQ(*x.insert_or_assign("hello"sv, "zzz"sv), "xxx");
        EXPECT_EQ(*x.try_emplace("hello"sv, "www"sv), "zzz");
    }

    {
        auto x = NewVector<String> {};
        auto y = NewVector<UniquePtr<int>> {};

        x.push_back("hello");
        y.push_back(make_unique<int>(42));
        x.push_back("world");
        y.push_back(make_unique<int>(100));

        auto z = collect_hash_map(zip(move_elements(move(x)), move_elements(move(y))));
        EXPECT_EQ(**z.at("hello"sv), 42);
        EXPECT_EQ(**z.erase("hello"sv), 42);
    }
}

static void format() {
    auto y = make_hash_map({ Pair { 20, 400 }, Pair { 400, 800 } });
    EXPECT_EQ(to_string(y), "{ 20: 400, 400: 800 }");
}

TEST_CONSTEXPR(hash_map, basic, basic)
TEST_CONSTEXPR(hash_map, containers, containers)
TEST_CONSTEXPR(hash_map, erase, erase)
TEST_CONSTEXPR(hash_map, comparison, comparison)
TEST_CONSTEXPR(hash_map, subcontainers, subcontainers)
TEST_SKIP(hash_map, transparent) {
    transparent();
}
TEST(hash_map, format) {
    format();
}
