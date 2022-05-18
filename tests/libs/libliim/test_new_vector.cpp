#include <liim/fixed_array.h>
#include <liim/new_vector.h>
#include <test/test.h>

constexpr void basic() {
    auto v = NewVector(5, 2);
    EXPECT_EQ(v[0], 2);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 2);
    EXPECT_EQ(v[3], 2);
    EXPECT_EQ(v[4], 2);
    EXPECT_EQ(v.size(), 5u);
    EXPECT(!v.empty());

    auto w = NewVector<int>();
    EXPECT_EQ(w.size(), 0u);
    EXPECT(w.empty());

    auto y = NewVector { 1, 2 };
    EXPECT_EQ(y.front(), 1);
    EXPECT_EQ(y.back(), 2);
}

constexpr void iterator() {
    int sum = 0;
    auto v = NewVector { 1, 2, 3, 4, 5 };
    for (auto& x : v) {
        sum += x;
    }
    EXPECT_EQ(sum, 15);
    for (auto it = v.crbegin(); it != v.crend(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 30);
}

constexpr void assign() {
    auto v = NewVector<int>(5u);
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(v[0], 0);
    v = { 2, 3, 4 };
    EXPECT_EQ(v[0], 2);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 4);
    EXPECT_EQ(v.size(), 3u);
    v.clear();
    EXPECT(v.empty());

    v = { 2, 3, 4 };
    auto w = v;
    EXPECT_EQ(w[0], 2);
    EXPECT_EQ(w[1], 3);
    EXPECT_EQ(w[2], 4);
    EXPECT_EQ(w.size(), 3u);

    auto z = NewVector<int> {};
    z = move(w);
    EXPECT(w.empty());
    EXPECT_EQ(z[0], 2);
    EXPECT_EQ(z[1], 3);
    EXPECT_EQ(z[2], 4);
    EXPECT_EQ(z.size(), 3u);
}

constexpr void mutate() {
    auto v = NewVector { 1, 3, 5 };
    EXPECT_EQ(v.size(), 3u);
    v.push_back(4);
    EXPECT_EQ(v.size(), 4u);
    EXPECT_EQ(v.back(), 4);
    EXPECT_EQ(v.pop_back(), 4);
    EXPECT_EQ(v.pop_back(), 5);
    EXPECT_EQ(v.pop_back(), 3);
    EXPECT_EQ(v.pop_back(), 1);
    EXPECT(!v.pop_back());

    v.assign(6, 1);
    EXPECT_EQ(v.size(), 6u);

    EXPECT_EQ(*v.insert(v.iterator(3), { 4, 5, 6 }), 4);
    EXPECT_EQ(v[3], 4);
    EXPECT_EQ(v[4], 5);
    EXPECT_EQ(v[5], 6);
    EXPECT_EQ(v.size(), 9u);
    EXPECT_EQ(v.erase(v.iterator(3), v.iterator(6)) - v.begin(), 3);
    EXPECT_EQ(v.size(), 6u);
    EXPECT(v.erase_count(3, 3) == v.end());
    EXPECT_EQ(v.size(), 3u);
}

TEST_CONSTEXPR(new_vector, basic, basic)
TEST_CONSTEXPR(new_vector, iterator, iterator)
TEST_CONSTEXPR(new_vector, assign, assign)
TEST_CONSTEXPR(new_vector, mutate, mutate)
