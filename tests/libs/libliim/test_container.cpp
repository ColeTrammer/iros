#include <liim/container/array.h>
#include <liim/container/container.h>
#include <liim/container/new_vector.h>
#include <liim/container/priority_queue.h>
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

constexpr void contains() {
    EXPECT(Alg::contains(range(5), 0));
    EXPECT(Alg::contains(range(5), 1));
    EXPECT(Alg::contains(range(5), 4));

    EXPECT(!Alg::contains(repeat(5, 1), 2));

    EXPECT(!Alg::contains(make_priority_queue({ 1, 2, 3 }), 4));
    EXPECT(Alg::contains(make_priority_queue({ 1, 2, 3 }), 2));

    EXPECT(Alg::contains(range(2, 7), "4"sv, [](int a, StringView b) {
        return a == b[0] - '0';
    }));

    struct X {
        int a;
    };
    EXPECT(Alg::contains(Array { X { 5 }, X { 7 } }, 5, Equal {}, &X::a));
}

constexpr void equal() {
    EXPECT(Alg::equal(range(5), range(5)));

    EXPECT(Alg::equal(Array { 2, 2, 2 }, repeat(3, 2)));
    EXPECT(!Alg::equal(Array { 2, 2, 2, 2 }, repeat(3, 2)));
    EXPECT(!Alg::equal(Array { 2, 2 }, repeat(3, 2)));
    EXPECT(!Alg::equal(Array { 2, 2 }, repeat(3, 2)));

    EXPECT(Alg::equal(make_priority_queue({ 3, 2, 1 }), Array { 1, 2, 3 }));

    EXPECT(Alg::equal(range(2, 7), Array { "2"sv, "3"sv, "4"sv, "5"sv, "6"sv }, [](int a, StringView b) {
        return a == b[0] - '0';
    }));

    struct X {
        int a;
    };
    struct Y {
        int b;
    };

    EXPECT(Alg::equal(Array { X { 3 }, X { 4 } }, Array { Y { 3 }, Y { 4 } }, Equal {}, &X::a, &Y::b));
}

constexpr void lexographic_compare() {
    EXPECT(Alg::lexographic_compare(range(5), range(5)) == 0);
    EXPECT(Alg::lexographic_compare(range(4), range(5)) < 0);
    EXPECT(Alg::lexographic_compare(range(6), range(5)) > 0);

    EXPECT(Alg::lexographic_compare(Array { 2, 3, 3 }, Array { 2, 3, 4 }) < 0);
    EXPECT(Alg::lexographic_compare(Array { 2, 3, 5 }, Array { 2, 3, 4 }) > 0);

    EXPECT(Alg::lexographic_compare(Array { 2, 3, 3 }, Array { 2, 3, 4 }, CompareThreeWayBackwards {}) > 0);
    EXPECT(Alg::lexographic_compare(Array { 2, 3, 5 }, Array { 2, 3, 4 }, CompareThreeWayBackwards {}) < 0);

    struct X {
        int a;
    };
    EXPECT(Alg::lexographic_compare(Array { X { 1 }, X { 2 }, X { 3 } }, Array { X { 1 }, X { 3 }, X { 3 } }, CompareThreeWay {}, &X::a,
                                    &X::a) < 0);
}

constexpr void sort() {
    auto v = make_vector({ 3, 5, 1, 2 });
    Alg::sort(v);
    EXPECT_EQ(v, make_vector({ 1, 2, 3, 5 }));

    auto w = make_vector({ 8, 7, 6, 5, 4, 3, 2, 1, 0 });
    Alg::sort(w);
    EXPECT_EQ(w, collect_vector(range(9)));

    Alg::sort(w, CompareThreeWayBackwards {});
    EXPECT_EQ(w, collect_vector(reversed(range(9))));

    auto x = collect_vector(range(128));
    Alg::sort(x, CompareThreeWayBackwards {});
    EXPECT_EQ(x, collect_vector(reversed(range(128))));
    EXPECT(Alg::is_sorted(reversed(x)));

    auto y = collect_vector(reversed(range(56)));
    insert(y, y.end(), range(56));
    Alg::sort(y);
    EXPECT(Alg::is_sorted(y));

    auto z = collect_vector(range(56));
    insert(z, z.end(), range(56));
    Alg::sort(z);
    EXPECT(Alg::is_sorted(z));

    EXPECT(Alg::is_sorted(range(1, 10)));
    EXPECT(!Alg::is_sorted(Array { 1, 2, 0 }));
    EXPECT(Alg::is_sorted(reversed(range(1, 10)), CompareThreeWayBackwards {}));

    EXPECT(Alg::is_sorted(collect_priority_queue(reversed(range(32)))));

    auto a = make_vector({ 5, 4, 3, 2, 1 });
    Alg::sort(transform(a, [](auto x) {
        return x + 1;
    }));
    EXPECT_EQ(a, make_vector({ 1, 2, 3, 4, 5 }));

    Alg::sort(reversed(a));
    EXPECT_EQ(a, make_vector({ 5, 4, 3, 2, 1 }));

    auto scores = make_vector({ 50, 20, 70, 10 });
    auto data = make_vector({ 37, 42, 60, 100, -1 });
    Alg::sort(zip(scores, data));

    EXPECT(Alg::is_sorted(zip(scores, data)));
    EXPECT_EQ(scores, make_vector({ 10, 20, 50, 70 }));
    EXPECT_EQ(data, make_vector({ 100, 42, 37, 60, -1 }));

    struct X {
        int a;
    };
    auto s = Array { X { 5 }, X { 4 }, X { 2 }, X { 3 } };
    Alg::sort(s, CompareThreeWay {}, &X::a);
    EXPECT(Alg::is_sorted(s, CompareThreeWay {}, &X::a));
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
    Alg::reverse(v);
    EXPECT_EQ(v, collect_vector(reversed(range(5))));

    assign_to(v, range(6));
    Alg::reverse(v);
    EXPECT_EQ(v, collect_vector(reversed(range(6))));

    auto w = NewVector<UniquePtr<int>> {};
    w.push_back(make_unique<int>(4));
    w.push_back(make_unique<int>(5));
    Alg::reverse(w);
    EXPECT_EQ(*w.front(), 5);
    EXPECT_EQ(*w.back(), 4);
}

constexpr void rotate() {
    auto v = collect_vector(range(5));
    auto r = Alg::rotate(v, v.iterator(2));
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
TEST_CONSTEXPR(container, contains, contains)
TEST_CONSTEXPR(container, equal, equal)
TEST_CONSTEXPR(container, lexographic_compare, lexographic_compare)
TEST_CONSTEXPR(container, sort, sort)
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
