#include <liim/container/container.h>
#include <liim/container/new_vector.h>
#include <test/test.h>

constexpr void collect() {
    auto v = collect<NewVector<int>>(range(5));
    EXPECT_EQ(v.clone(), make_vector({ 0, 1, 2, 3, 4 }));

    auto x = NewVector<UniquePtr<int>> {};
    x.push_back(make_unique<int>(43));
    x.push_back(make_unique<int>(45));
    auto w = collect_vector(move(x));
    EXPECT_EQ(*w[0], 43);
    EXPECT_EQ(*w[1], 45);
    EXPECT_EQ(w.size(), 2u);

    const auto r = repeat(3lu, 5);
    auto z = collect_vector(r);
    EXPECT_EQ(z.clone(), make_vector({ 5, 5, 5 }));
}

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

constexpr void reverse() {
    auto v = collect_vector(range(5));
    reverse(v);
    EXPECT_EQ(v, collect_vector(reversed(range(5))));

    assign_to(v, range(6));
    reverse(v);
    EXPECT_EQ(v, collect_vector(reversed(range(6))));

    auto w = NewVector<UniquePtr<int>> {};
    w.push_back(make_unique<int>(4));
    w.push_back(make_unique<int>(5));
    reverse(w);
    EXPECT_EQ(*w.front(), 5);
    EXPECT_EQ(*w.back(), 4);
}

constexpr void rotate() {
    auto v = collect_vector(range(5));
    auto r = rotate(v, v.iterator(2));
    EXPECT_EQ(v, make_vector({ 2, 3, 4, 0, 1 }));
    EXPECT_EQ(v.iterator_index(r), 3u);
}

constexpr void enumerate() {
    auto v = make_vector<size_t>({ 5lu, 6lu, 7lu, 8lu });
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

constexpr void transform() {
    for (auto [x, y] : zip(transform(range(5),
                                     [](auto x) {
                                         return x + 1;
                                     }),
                           range(1, 6))) {
        EXPECT_EQ(x, y);
    }

    auto w = NewVector<UniquePtr<size_t>> {};
    w.push_back(make_unique<size_t>(42));
    w.push_back(make_unique<size_t>(84));
    for (auto [x, y] : enumerate(transform(w, [](auto& p) {
             return *p;
         }))) {
        EXPECT_EQ((x + 1) * 42, y);
    }
    for (auto [x, y] : enumerate(transform(move_elements(move(w)), [](auto&& p) {
             return *p;
         }))) {
        EXPECT_EQ((x + 1) * 42, y);
    }

    {
        int sum = 0;
        for (auto x : transform(FixedArray { 3, 4, 5 }, [](auto x) {
                 return x * 2;
             })) {
            sum += x;
        }
        EXPECT_EQ(sum, 24);
    }
    {
        int sum = 0;
        auto m = transform(FixedArray { 3, 4, 5 }, [](auto x) {
            return x * 2;
        });
        for (auto x : m) {
            sum += x;
        }
        EXPECT_EQ(sum, 24);
    }
}

constexpr void zip() {
    auto v = make_vector({ 2, 4, 6 });
    auto w = make_vector({ 6, 4, 2 });

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

    for (auto [a, b] : zip(y, z)) {
        EXPECT_EQ(*a + *b, 8);
    }
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
    auto v = make_vector({ 2, 3, 4 });
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
    for (auto& x : reversed(l)) {
        v.push_back(x);
    }
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 4);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 2);
}

constexpr void value_iterator() {
    struct Iter : public ValueIteratorAdapter<Iter> {
        using ValueType = size_t;
        constexpr Option<ValueType> next() { return i++; }

        ValueType i { 0 };
    };

    size_t sum = 0;
    for (auto [i, x] : enumerate(Iter {})) {
        EXPECT_EQ(i, x);
        sum += x;
        if (x == 10) {
            break;
        }
    }
    EXPECT_EQ(sum, 10lu * 11lu / 2lu);

    struct Iter2 : public ValueIteratorAdapter<Iter2> {
        using ValueType = size_t;
        constexpr Option<ValueType> next() {
            if (i <= 10) {
                return i++;
            }
            return None {};
        }

        ValueType i { 0 };
    };

    sum = 0;
    for (auto x : Iter2 {}) {
        sum += x;
    }
    EXPECT_EQ(sum, 10lu * 11lu / 2lu);

    struct Iter3 : public ValueIteratorAdapter<Iter3> {
        using ValueType = size_t&;
        constexpr Option<size_t&> next() {
            if (i < 10) {
                ++i;
                return i;
            }
            return None {};
        }

        size_t i { 0 };
    };

    sum = 0;
    for (auto& x : Iter3 {}) {
        sum += x;
    }
    EXPECT_EQ(sum, 10lu * 11lu / 2lu);
}

TEST_CONSTEXPR(container, collect, collect)
TEST_CONSTEXPR(container, range, range)
TEST_CONSTEXPR(container, repeat, repeat)
TEST_CONSTEXPR(container, reversed, reversed)
TEST_CONSTEXPR(container, reverse, reverse)
TEST_CONSTEXPR(container, rotate, rotate)
TEST_CONSTEXPR(container, enumerate, enumerate)
TEST_CONSTEXPR(container, transform, transform)
TEST_CONSTEXPR(container, zip, zip)
TEST_CONSTEXPR(container, move_elements, move_elements)
TEST_CONSTEXPR(container, iterator_container, iterator_container)
TEST_CONSTEXPR(container, initializer_list, initializer_list)
TEST_CONSTEXPR(container, value_iterator, value_iterator)
