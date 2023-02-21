#include <di/concepts/same_as.h>
#include <di/meta/add_lvalue_reference.h>
#include <di/meta/add_rvalue_reference.h>
#include <di/meta/decay.h>
#include <di/meta/index_sequence.h>
#include <di/meta/like.h>
#include <di/meta/make_index_sequence.h>
#include <di/prelude.h>

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

static_assert(di::SameAs<di::meta::MakeIndexSequence<6>, di::meta::IndexSequence<0, 1, 2, 3, 4, 5>>);
static_assert(di::SameAs<di::meta::MakeIndexSequence<5>, di::meta::IndexSequence<0, 1, 2, 3, 4>>);
static_assert(di::SameAs<di::meta::MakeIndexSequence<4>, di::meta::IndexSequence<0, 1, 2, 3>>);
static_assert(di::SameAs<di::meta::MakeIndexSequence<3>, di::meta::IndexSequence<0, 1, 2>>);
static_assert(di::SameAs<di::meta::MakeIndexSequence<2>, di::meta::IndexSequence<0, 1>>);
static_assert(di::SameAs<di::meta::MakeIndexSequence<1>, di::meta::IndexSequence<0>>);
static_assert(di::SameAs<di::meta::MakeIndexSequence<0>, di::meta::IndexSequence<>>);

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
    using Invoke = di::meta::BoolConstant<!di::SameAs<T, i32>>;
};

static_assert(di::SameAs<di::meta::Filter<di::meta::List<i32, i64, i32, i16, i32>, P>, di::meta::List<i64, i16>>);

static_assert(di::SameAs<di::meta::Transform<di::meta::List<i32, i64>,
                                             di::meta::BindBack<di::meta::Quote<di::meta::AddLValueReference>>>,
                         di::meta::List<i32&, i64&>>);

static_assert(di::SameAs<di::meta::TupleElements<di::Tuple<int, long>>, di::meta::List<int, long>>);
static_assert(di::SameAs<di::Tuple<long>, di::meta::Type<di::meta::CustomCommonType<di::Tuple<int>, di::Tuple<long>>>>);
static_assert(di::SameAs<int const&, di::meta::CommonReference<int const&, int&&>>);
static_assert(di::SameAs<di::Tuple<int const&>, di::meta::CommonReference<di::Tuple<int const&>, di::Tuple<int&&>>>);
