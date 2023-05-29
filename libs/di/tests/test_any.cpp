#include <di/any/prelude.h>
#include <di/any/storage/prelude.h>
#include <di/concepts/prelude.h>
#include <di/types/integers.h>
#include <di/util/prelude.h>
#include <di/vocab/array/prelude.h>
#include <di/vocab/pointer/prelude.h>
#include <dius/test/prelude.h>

namespace any {
struct X : di::Dispatcher<X, i32(di::This const&, i32)> {};

constexpr inline auto xf = X {};

struct A {};
struct B {
    B() : nothing(di::make_box<i32>(42)) {}

    di::Array<di::Byte, 16> padding {};
    di::Box<i32> nothing;
};

i32 tag_invoke(X, i32 const& x, i32 y) {
    return x + y;
}

i32 tag_invoke(X, A const&, i32 y) {
    return y + 4;
}

i32 tag_invoke(X, B const&, i32 y) {
    return y + 5;
}

constexpr inline auto return_1 = [](auto&) {
    return 1;
};

struct Y : di::Dispatcher<Y, i32(di::This&), decltype(return_1)> {};

constexpr inline auto yf = Y {};

i32 tag_invoke(Y, i32& x) {
    return x + 2;
}

using XM = di::meta::Type<X>;
using YM = di::Method<Y, i32(di::This&)>;

using Interface = di::meta::List<X, YM>;

constexpr void meta() {
    static_assert(di::concepts::Interface<Interface>);

    using S = di::meta::MethodErasedSignature<di::meta::Type<X>>;
    static_assert(di::SameAs<S, i32(void*, i32)>);

    static_assert(di::SameAs<di::meta::MethodErasedSignature<di::Method<Y, i32(i32, di::String const&, di::This&&)>>,
                             i32(i32, di::String const&, void*)>);

    static_assert(di::concepts::MethodCallableWith<XM, i32>);
    static_assert(di::concepts::MethodCallableWith<YM, i32>);
}

constexpr void vtable() {
    using Storage = di::any::RefStorage;

    using VTable = di::any::InlineVTable::Invoke<Interface>;

    /* di::concepts::VTableFor<Interface> */ auto vtable = VTable::create_for<Storage, i32&>();
    (void) vtable;

    using VTable2 = di::any::OutOfLineVTable::Invoke<Interface>;

    /* di::concepts::VTableFor<Interface> */ auto vtable2 = VTable2::create_for<Storage, i32&>();
    (void) vtable2;
}

static void ref() {
    using Any = di::any::AnyRef<Interface>;

    i32 v = 4;
    auto x = Any(v);

    ASSERT_EQ(xf(x, 12), 16);
    ASSERT_EQ(yf(x), 6);

    auto z = x;

    ASSERT_EQ(xf(z, 12), 16);
    ASSERT_EQ(yf(z), 6);

    auto a = A {};
    auto y = Any(a);

    ASSERT_EQ(xf(y, 12), 16);
    ASSERT_EQ(yf(y), 1);
}

static void inline_() {
    using Any = di::any::AnyInline<Interface>;

    auto x = Any(4);

    ASSERT_EQ(xf(x, 12), 16);
    ASSERT_EQ(yf(x), 6);

    auto y = Any(di::in_place_type<A>);

    ASSERT_EQ(xf(y, 12), 16);
    ASSERT_EQ(yf(y), 1);

    auto z = di::move(y);
    ASSERT(!y.has_value());

    ASSERT_EQ(xf(z, 12), 16);
    ASSERT_EQ(yf(z), 1);

    z = di::move(x);
    ASSERT(!x.has_value());

    ASSERT_EQ(xf(z, 12), 16);
    ASSERT_EQ(yf(z), 6);

    z = 3;
    ASSERT_EQ(xf(z, 12), 15);
    ASSERT_EQ(yf(z), 5);
}

static void unique() {
    using Any = di::any::AnyUnique<Interface>;

    auto x = Any::create(4);

    ASSERT_EQ(xf(x, 12), 16);
    ASSERT_EQ(yf(x), 6);

    auto y = Any::create(di::in_place_type<A>);

    ASSERT_EQ(xf(y, 12), 16);
    ASSERT_EQ(yf(y), 1);

    auto z = di::move(y);
    ASSERT(!y.has_value());

    ASSERT_EQ(xf(z, 12), 16);
    ASSERT_EQ(yf(z), 1);

    z = di::move(x);
    ASSERT(!x.has_value());

    ASSERT_EQ(xf(z, 12), 16);
    ASSERT_EQ(yf(z), 6);

    z = Any::create(3);

    ASSERT_EQ(xf(z, 12), 15);
    ASSERT_EQ(yf(z), 5);
}

static void hybrid() {
    using Any = di::Any<Interface>;

    auto x = Any(4);

    ASSERT_EQ(xf(x, 12), 16);
    ASSERT_EQ(yf(x), 6);

    auto y = Any(di::in_place_type<A>);

    ASSERT_EQ(xf(y, 12), 16);
    ASSERT_EQ(yf(y), 1);

    auto z = di::move(y);
    ASSERT(!y.has_value());

    ASSERT_EQ(xf(z, 12), 16);
    ASSERT_EQ(yf(z), 1);

    z = di::move(x);
    ASSERT(!x.has_value());

    ASSERT_EQ(xf(z, 12), 16);
    ASSERT_EQ(yf(z), 6);

    z = Any(3);

    ASSERT_EQ(xf(z, 12), 15);
    ASSERT_EQ(yf(z), 5);

    auto q = Any::create(B {});

    ASSERT_EQ(xf(q, 12), 17);
    ASSERT_EQ(yf(q), 1);
}

static void shared() {
    using Any = di::any::AnyShared<Interface>;

    auto x = Any::create(4);

    ASSERT_EQ(xf(x, 12), 16);
    ASSERT_EQ(yf(x), 6);

    auto y = Any::create(di::in_place_type<A>);

    ASSERT_EQ(xf(y, 12), 16);
    ASSERT_EQ(yf(y), 1);

    auto z = di::move(y);
    ASSERT(!y.has_value());

    ASSERT_EQ(xf(z, 12), 16);
    ASSERT_EQ(yf(z), 1);

    z = di::move(x);
    ASSERT(!x.has_value());

    ASSERT_EQ(xf(z, 12), 16);
    ASSERT_EQ(yf(z), 6);

    z = Any::create(3);

    ASSERT_EQ(xf(z, 12), 15);
    ASSERT_EQ(yf(z), 5);

    auto p = z;
    ASSERT_EQ(xf(p, 12), 15);
    ASSERT_EQ(yf(p), 5);
}

struct Z : di::Immovable {
    explicit Z(i32 x_) : x(x_) {}

    int x;
};

i32 tag_invoke(X, Z const& z, i32 y) {
    return y + z.x;
}

i32 tag_invoke(Y, di::meta::Type<Y>, Z& z) {
    return 4 + z.x;
}

static void immovable() {
    using Any = di::any::AnyHybrid<Interface, di::StorageCategory::Immovable>;

    auto x = Any(di::in_place_type<Z>, 3);

    ASSERT_EQ(xf(x, 12), 15);
    ASSERT_EQ(yf(x), 7);

    x.emplace(di::in_place_type<Z>, 4);

    ASSERT_EQ(xf(x, 12), 16);
    ASSERT_EQ(yf(x), 8);

    x.emplace(4);

    ASSERT_EQ(xf(x, 12), 16);
    ASSERT_EQ(yf(x), 6);
}

struct ZF : di::Dispatcher<ZF, void(di::This&&, i32)> {};

void tag_invoke(ZF, A&&, i32 x) {
    ASSERT_EQ(x, 12);
}

constexpr inline auto zf = ZF {};

static void rvalue() {
    using Any = di::Any<di::meta::List<ZF>>;

    auto x = Any(di::in_place_type<A>);

    zf(di::move(x), 12);
}

TESTC(any, meta)
TESTC(any, vtable)
TEST(any, ref)
TEST(any, inline_)
TEST(any, unique)
TEST(any, hybrid)
TEST(any, shared)
TEST(any, immovable)
TEST(any, rvalue)
}
