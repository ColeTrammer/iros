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
