#include "di/meta/algorithm.h"
#include "di/meta/common.h"
#include "di/meta/constexpr.h"
#include "di/meta/util.h"
#include "di/types/prelude.h"
#include "di/vocab/tuple/prelude.h"

namespace meta {
static_assert(di::SameAs<di::meta::Decay<int const&&>, int>);
static_assert(di::SameAs<di::meta::Decay<bool[42]>, bool*>);

static_assert(di::SameAs<di::meta::AddRValueReference<int>, int&&>);
static_assert(di::SameAs<di::meta::AddRValueReference<int&>, int&>);
static_assert(di::SameAs<di::meta::AddRValueReference<int&&>, int&&>);
static_assert(di::SameAs<di::meta::AddRValueReference<void const>, void const>);

static_assert(di::SameAs<di::meta::AddLValueReference<int>, int&>);
static_assert(di::SameAs<di::meta::AddLValueReference<int&>, int&>);
static_assert(di::SameAs<di::meta::AddLValueReference<int&&>, int&>);
static_assert(di::SameAs<di::meta::AddLValueReference<void const>, void const>);

static_assert(di::SameAs<di::meta::Like<int const&, i32>, i32 const&>);
static_assert(di::SameAs<di::meta::Like<int const&&, i32>, i32 const&&>);
static_assert(di::SameAs<di::meta::Like<int&&, i32>, i32&&>);
static_assert(di::SameAs<di::meta::Like<int&, i32>, i32&>);
static_assert(di::SameAs<di::meta::Like<int, i32>, i32>);

static_assert(di::SameAs<di::meta::MakeIndexSequence<6>, di::meta::ListV<0ZU, 1ZU, 2ZU, 3ZU, 4ZU, 5ZU>>);
static_assert(di::SameAs<di::meta::MakeIndexSequence<5>, di::meta::ListV<0ZU, 1ZU, 2ZU, 3ZU, 4ZU>>);
static_assert(di::SameAs<di::meta::MakeIndexSequence<4>, di::meta::ListV<0ZU, 1ZU, 2ZU, 3ZU>>);
static_assert(di::SameAs<di::meta::MakeIndexSequence<3>, di::meta::ListV<0ZU, 1ZU, 2ZU>>);
static_assert(di::SameAs<di::meta::MakeIndexSequence<2>, di::meta::ListV<0ZU, 1ZU>>);
static_assert(di::SameAs<di::meta::MakeIndexSequence<1>, di::meta::ListV<0ZU>>);
static_assert(di::SameAs<di::meta::MakeIndexSequence<0>, di::meta::ListV<>>);

static_assert(
    di::SameAs<unsigned long long,
               di::meta::CommonType<unsigned char, unsigned short, unsigned, unsigned long, unsigned long long>>);

static_assert(
    di::SameAs<di::meta::List<di::meta::List<u8, u16, u32>, di::meta::List<u8, u16, i32>, di::meta::List<u8, i16, u32>,
                              di::meta::List<u8, i16, i32>, di::meta::List<i8, u16, u32>, di::meta::List<i8, u16, i32>,
                              di::meta::List<i8, i16, u32>, di::meta::List<i8, i16, i32>>,
               di::meta::CartesianProduct<di::meta::List<u8, i8>, di::meta::List<u16, i16>, di::meta::List<u32, i32>>>);

struct P {
    template<typename T>
    using Invoke = di::Constexpr<!di::SameAs<T, i32>>;
};

static_assert(di::SameAs<di::meta::Filter<di::meta::List<i32, i64, i32, i16, i32>, P>, di::meta::List<i64, i16>>);

static_assert(di::SameAs<di::meta::Transform<di::meta::List<i32, i64>,
                                             di::meta::BindBack<di::meta::Quote<di::meta::AddLValueReference>>>,
                         di::meta::List<i32&, i64&>>);

static_assert(di::SameAs<di::meta::TupleElements<di::Tuple<int, long>>, di::meta::List<int, long>>);
static_assert(di::SameAs<di::Tuple<long>, di::meta::Type<di::meta::CustomCommonType<di::Tuple<int>, di::Tuple<long>>>>);
static_assert(di::SameAs<int const&, di::meta::CommonReference<int const&, int&&>>);
static_assert(di::SameAs<di::Tuple<int const&>, di::meta::CommonReference<di::Tuple<int const&>, di::Tuple<int&&>>>);
}

// Test constexpr wrapper
static_assert(di::c_<32> == 32);
static_assert((di::c_<32> + di::c_<52>) == 84);
static_assert((di::c_<50> - di::c_<20>) == 30);
static_assert((di::c_<2> * di::c_<3>) == 6);
static_assert((di::c_<6> / di::c_<2>) == 3);
static_assert((di::c_<6> % di::c_<4>) == 2);
static_assert((di::c_<2> << di::c_<3>) == 16);
static_assert((di::c_<16> >> di::c_<3>) == 2);
static_assert((di::c_<2> & di::c_<3>) == 2);
static_assert((di::c_<2> | di::c_<3>) == 3);
static_assert((di::c_<2> ^ di::c_<3>) == 1);
static_assert((~di::c_<2>) == -3);
static_assert((di::c_<2> == di::c_<2>) );
static_assert((di::c_<2> != di::c_<3>) );
static_assert((di::c_<2> < di::c_<3>) );
static_assert((di::c_<2> <= di::c_<3>) );
static_assert((di::c_<3> > di::c_<2>) );
static_assert((di::c_<3> >= di::c_<2>) );
static_assert((di::c_<true> && di::c_<true>) );
static_assert((di::c_<true> || di::c_<false>) );
static_assert((!di::c_<false>) );
