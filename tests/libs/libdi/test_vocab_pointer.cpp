#include <di/prelude.h>
#include <test/test.h>

struct X {
    int x;
};

struct Y : X {
    constexpr explicit Y(int x_, int y_) : X(x_), y(y_) {}

    int y;
};

constexpr void box() {
    auto x = di::make_box<i32>(42);
    ASSERT_EQ(*x, 42);

    auto y = di::make_box<Y>(12, 42);
    ASSERT_EQ(y->x, 12);

    auto a = di::Box<X>(di::move(y));
    ASSERT_EQ(a->x, 12);

    a = di::make_box<Y>(13, 43);
    ASSERT_EQ(a->x, 13);

    ASSERT_NOT_EQ(x, y);
    ASSERT_NOT_EQ(x, nullptr);
    ASSERT_EQ(nullptr, y);
    ASSERT((y <=> nullptr) == 0);

    auto z = *di::try_box<int>(13);
    ASSERT_EQ(*z, 13);
}

TEST_CONSTEXPR(vocab_pointer, box, box)