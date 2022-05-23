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

    EXPECT(y.at(0));
    EXPECT_EQ(*y.at(0), 1);
    EXPECT(y.at(1));
    EXPECT_EQ(*y.at(1), 2);
    EXPECT(!y.at(2));
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
    auto a = v.erase_count(3, 3);
    EXPECT(a == v.end());
    EXPECT_EQ(v.size(), 3u);

    v.resize(201);
    EXPECT_EQ(v.size(), 201u);
    EXPECT_EQ(v.back(), 0);
    v.resize(2);
    EXPECT_EQ(v.size(), 2u);

    v = { 1, 2, 3, 4 };
    v.erase_unstable(1);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 4);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(v.size(), 3u);
}

constexpr void container() {
    // FIXME: allow passing the range directory without creating a tempory variable
    auto r = range(2, 6);
    auto v = NewVector<int> { r };
    EXPECT_EQ(v.size(), 4u);
    EXPECT_EQ(v.front(), 2);

    v.insert(1, repeat(3, 7));
    EXPECT_EQ(v.size(), 7u);
    EXPECT_EQ(v[0], 2);
    EXPECT_EQ(v[1], 7);
    EXPECT_EQ(v[2], 7);
    EXPECT_EQ(v[3], 7);
    EXPECT_EQ(v[4], 3);

    auto rr = range(3);
    v.assign(rr);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 0);
    EXPECT_EQ(v[1], 1);
    EXPECT_EQ(v[2], 2);

    auto rrr = range(2);
    v = rrr;
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], 0);
    EXPECT_EQ(v[1], 1);

    auto w = NewVector<UniquePtr<int>> {};
    w.push_back(make_unique<int>(4));
    w.push_back(make_unique<int>(8));

    auto z = NewVector<UniquePtr<int>> {};
    z.push_back(make_unique<int>(6));

    w.insert(1, move(z));
    EXPECT_EQ(w.size(), 3u);
    EXPECT_EQ(*w[1], 6);
}

TEST_CONSTEXPR(new_vector, basic, basic)
TEST_CONSTEXPR(new_vector, iterator, iterator)
TEST_CONSTEXPR(new_vector, assign, assign)
TEST_CONSTEXPR(new_vector, mutate, mutate)
TEST_CONSTEXPR(new_vector, container, container)
