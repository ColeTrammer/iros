#include <di/concepts/same_as.h>
#include <di/meta/add_lvalue_reference.h>
#include <di/meta/add_rvalue_reference.h>
#include <di/meta/decay.h>
#include <di/meta/like.h>
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
