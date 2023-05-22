#include <di/vocab/pointer/prelude.h>
#include <dius/test/prelude.h>

namespace vocab_pointer {
struct X {
    constexpr explicit X(int x_) : x(x_) {}

    constexpr virtual ~X() {}

    int x;
};

struct Y : X {
    constexpr explicit Y(int x_, int y_) : X(x_), y(y_) {}

    constexpr virtual ~Y() override {}

    int y;
};

constexpr void box() {
    auto x = di::make_box<i32>(42);
    ASSERT_EQ(*x, 42);

    auto y = di::make_box<Y>(12, 42);
    ASSERT_EQ(y->x, 12);

    ASSERT_NOT_EQ(x, y);
    ASSERT_NOT_EQ(x, nullptr);

    auto z = di::make_box<int>(13);

    auto w = di::make_box<di::Box<int>>(di::move(z));
    ASSERT_EQ(**w, 13);
}

constexpr void rc() {
    struct X : di::IntrusiveThreadUnsafeRefCount<X> {
    private:
        friend di::IntrusiveThreadUnsafeRefCount<X>;

        constexpr explicit X(int value_) : value(value_) {}

    public:
        int value;
    };

    auto x = di::make_rc<X>(42);
    ASSERT_EQ(x->value, 42);

    auto y = x;
    auto z = x->rc_from_this();

    ASSERT_EQ(y, z);
    ASSERT_NOT_EQ(y, nullptr);
}

static void arc() {
    struct X : di::IntrusiveRefCount<X> {
    private:
        friend di::IntrusiveRefCount<X>;

        constexpr explicit X(int value_) : value(value_) {}

    public:
        int value;
    };

    auto x = di::make_arc<X>(42);
    ASSERT_EQ(x->value, 42);

    auto y = x;
    auto z = x->arc_from_this();

    ASSERT_EQ(y, z);
    ASSERT_NOT_EQ(y, nullptr);
}

TESTC(vocab_pointer, box)
TESTC(vocab_pointer, rc)
TEST(vocab_pointer, arc)
}
