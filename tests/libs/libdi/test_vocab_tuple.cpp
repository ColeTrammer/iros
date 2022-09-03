#include <di/concepts/decay_same_as.h>
#include <di/meta/add_member_get.h>
#include <di/meta/like.h>
#include <di/prelude.h>
#include <di/util/forward_like.h>
#include <test/test.h>

class X : public di::meta::AddMemberGet<X> {
public:
private:
    constexpr friend size_t tag_invoke(di::Tag<di::vocab::tuple::tuple_size>, di::InPlaceType<X>) { return 1zu; }

    constexpr friend int tag_invoke(di::Tag<di::vocab::tuple::tuple_element>, di::InPlaceType<X>, di::InPlaceIndex<0>);

    template<di::concepts::DecaySameAs<X> Self>
    constexpr friend di::meta::Like<Self, int> tag_invoke(di::Tag<di::util::get_in_place>, di::InPlaceIndex<0>, Self&& self) {
        return di::util::forward_like<Self>(self.x);
    }

    int x;
};

static_assert(di::concepts::detail::HasTupleElement<X, 0>);
static_assert(di::concepts::detail::HasTupleGet<X, 0>);

static_assert(di::concepts::TupleLike<X>);
static_assert(di::concepts::detail::CanStructuredBind<X>);

constexpr void enable_structed_bindings() {
    auto x = X {};
    auto [y] = x;

    (void) x.get<0>();

    (void) y;
}

struct XX {
    int x;
    int y;
    long z;
};

struct XXX {
    int x;
    long y;
    int z;
};

struct XXXX {
    long x;
    int y;
    int z;
};

struct XXXXX {
    int x;
    long y;
    int z;
    long xx;
    int yy;
    long zz;
};

static_assert(sizeof(XX) == sizeof(di::Tuple<XX>));
static_assert(sizeof(XX) == sizeof(di::Tuple<int, int, long>));
static_assert(sizeof(XXX) == sizeof(di::Tuple<int, long, int>));
static_assert(sizeof(XXXX) == sizeof(di::Tuple<long, int, int>));
static_assert(sizeof(XXXXX) == sizeof(di::Tuple<int, long, int, long, int, long>));

constexpr void basic() {
    static_assert(di::concepts::TupleLike<di::Tuple<int, int, int>>);

    auto x = di::Tuple<int, int, int> {};

    auto e = di::get<2>(x);
    EXPECT_EQ(e, 0);

    auto f = x.get<0>();
    EXPECT_EQ(f, 0);

    static_assert(di::concepts::detail::CanStructuredBindHelper<di::Tuple<int, int, int>, di::meta::IndexSequence<0, 1, 2>>::value);
    static_assert(di::concepts::detail::CanStructuredBind<di::Tuple<int, int, int>>);

    auto [a, b, c] = x;
    EXPECT_EQ(a, 0);
    EXPECT_EQ(b, 0);
    EXPECT_EQ(c, 0);

    using Z = di::vocab::tuple::TupleImpl<di::meta::IndexSequenceFor<int, int, int>, int, int, int>;
    static_assert(
        di::concepts::ConstructibleFrom<Z, di::vocab::tuple::ConstructTupleImplFromTuplelike, di::Tuple<short, short, int> const&>);
    static_assert(di::concepts::ConstructibleFrom<di::Tuple<int, int, int>, di::Tuple<short, short, int> const&>);
    static_assert(di::concepts::ConstructibleFrom<di::Tuple<int, int, int>, di::Tuple<short, short, int>&>);
    static_assert(di::concepts::ConstructibleFrom<di::Tuple<int, int, int>, di::Tuple<short, short, int> const&&>);
    static_assert(di::concepts::ConstructibleFrom<di::Tuple<int, int, int>, di::Tuple<short, short, int>&&>);

    auto y = di::Tuple<int, int, int>(di::Tuple<short, short, int>(1, 2, 3));
    auto [i, j, k] = y;
    EXPECT_EQ(i, 1);
    EXPECT_EQ(j, 2);
    EXPECT_EQ(k, 3);
}

constexpr void assignment() {
    int a = 2, b = 3, c = 4;
    const auto x = di::tie(a, b, c);

    int d = 5, e = 6, f = 7;
    x = di::tie(d, e, f);

    EXPECT_EQ(a, 5);
    EXPECT_EQ(e, 6);
    EXPECT_EQ(f, 7);

    auto y = di::make_tuple(6l, 5l, 4l);
    y = di::make_tuple(4l, 3l, 2l);
    EXPECT_EQ(di::get<0>(y), 4);
    EXPECT_EQ(di::get<1>(y), 3);
    EXPECT_EQ(di::get<2>(y), 2);

    y = di::make_tuple(3, 2, 1);
    EXPECT_EQ(di::get<0>(y), 3);
    EXPECT_EQ(di::get<1>(y), 2);
    EXPECT_EQ(di::get<2>(y), 1);
}

TEST_CONSTEXPR(vocab_tuple, enable_structed_bindings, enable_structed_bindings)
TEST_CONSTEXPR(vocab_tuple, basic, basic)
TEST_CONSTEXPR(vocab_tuple, assignment, assignment)
