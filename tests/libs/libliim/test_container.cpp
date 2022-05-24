#include <liim/container.h>
#include <liim/new_vector.h>
#include <test/test.h>

constexpr void range() {
    auto v = NewVector<int> {};
    for (auto i : range(5)) {
        v.push_back(i);
    }
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(v[0], 0);
    EXPECT_EQ(v[1], 1);
    EXPECT_EQ(v[2], 2);
    EXPECT_EQ(v[3], 3);
    EXPECT_EQ(v[4], 4);

    v.clear();
    for (auto i : range(3, 5)) {
        v.push_back(i);
    }
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], 3);
    EXPECT_EQ(v[1], 4);

    v.clear();
    for (auto i : range(-1)) {
        v.push_back(i);
    }
    EXPECT_EQ(v.size(), 0u);

    for (auto i : range(4, 2)) {
        v.push_back(i);
    }
    EXPECT_EQ(v.size(), 0u);
}

constexpr void repeat() {
    auto v = NewVector<int> {};
    for (auto i : repeat(5, 2)) {
        v.push_back(i);
    }
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(v[0], 2);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 2);
    EXPECT_EQ(v[3], 2);
    EXPECT_EQ(v[4], 2);
}

constexpr void reversed() {
    auto v = NewVector<int> {};
    for (auto i : reversed(range(5))) {
        v.push_back(i);
    }
    EXPECT_EQ(v.size(), 5u);
    EXPECT_EQ(v[0], 4);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 2);
    EXPECT_EQ(v[3], 1);
    EXPECT_EQ(v[4], 0);

    auto w = NewVector<int> {};
    for (auto& i : reversed(v)) {
        w.push_back(i);
    }
    EXPECT_EQ(w.size(), 5u);
    EXPECT_EQ(w[0], 0);
    EXPECT_EQ(w[1], 1);
    EXPECT_EQ(w[2], 2);
    EXPECT_EQ(w[3], 3);
    EXPECT_EQ(w[4], 4);
}

constexpr void enumerate() {
    auto v = NewVector<size_t> { 5lu, 6lu, 7lu, 8lu };
    for (auto [i, x] : enumerate(v)) {
        EXPECT_EQ(i + 5, x);
        ++x;
    }
    for (auto [i, x] : enumerate(v)) {
        EXPECT_EQ(i + 6, x);
    }

    auto w = NewVector<UniquePtr<size_t>> {};
    w.push_back(make_unique<size_t>(10));
    w.push_back(make_unique<size_t>(20));
    for (auto [i, p] : enumerate(w)) {
        EXPECT_EQ((i + 1) * 10, *p);
    }
    for (auto [i, p] : enumerate(move_elements(move(w)))) {
        EXPECT_EQ((i + 1) * 10, *p);
    }

    for (auto [i, x] : enumerate(range(5lu))) {
        EXPECT_EQ(i, x);
    }
}

constexpr void zip() {
    auto v = NewVector { 2, 4, 6 };
    auto w = NewVector { 6, 4, 2 };

    for (auto [a, b] : zip(v, w)) {
        EXPECT_EQ(a + b, 8);
    }

    int sum = 0;
    for (auto [a, b, c] : zip(range(5), range(3), range(2))) {
        sum += a + b + c;
    }
    EXPECT_EQ(sum, 3);

    auto y = NewVector<UniquePtr<int>> {};
    auto z = NewVector<UniquePtr<int>> {};
    y.push_back(make_unique<int>(6));
    y.push_back(make_unique<int>(2));

    for (auto [a, b] : zip(move_elements(move(y)), move_elements(move(z)))) {
        EXPECT_EQ(*a + *b, 8);
    }
}

constexpr void move_elements() {
    auto v = NewVector<UniquePtr<int>> {};
    v.push_back(make_unique<int>(42));
    v.push_back(make_unique<int>(20));

    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(*v[0], 42);
    EXPECT_EQ(*v[1], 20);

    auto w = NewVector<UniquePtr<int>> {};
    for (auto&& x : move_elements(move(v))) {
        w.push_back(move(x));
    }

    EXPECT_EQ(w.size(), 2u);
    EXPECT_EQ(*w[0], 42);
    EXPECT_EQ(*w[1], 20);
}

constexpr void iterator_container() {
    auto v = NewVector { 2, 3, 4 };
    auto c = iterator_container(v.begin(), v.end());
    EXPECT_EQ(c.size(), 3);
    auto it = c.begin();
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 4);
}

constexpr void initializer_list() {
    auto l = std::initializer_list<int> { 2, 3, 4 };
    EXPECT_EQ(l.size(), 3u);
    auto it = l.begin();
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 4);

    auto v = NewVector<int> {};
    for (auto& x : LIIM::reversed(l)) {
        v.push_back(x);
    }
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 4);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 2);
}

TEST_CONSTEXPR(container, range, range)
TEST_CONSTEXPR(container, repeat, repeat)
TEST_CONSTEXPR(container, reversed, reversed)
TEST_CONSTEXPR(container, enumerate, enumerate)
TEST_CONSTEXPR(container, zip, zip)
TEST_CONSTEXPR(container, move_elements, move_elements)
TEST_CONSTEXPR(container, iterator_container, iterator_container)
TEST_CONSTEXPR(container, initializer_list, initializer_list)
