#include <di/prelude.h>
#include <di/util/concepts/same_as.h>
#include <di/util/meta/add_lvalue_reference.h>
#include <di/util/meta/add_rvalue_reference.h>
#include <di/util/meta/decay.h>
#include <di/util/meta/like.h>

static_assert(di::conc::SameAs<di::meta::Decay<int const&&>, int>);
static_assert(di::conc::SameAs<di::meta::Decay<bool[42]>, bool*>);

static_assert(di::conc::SameAs<di::meta::AddRValueReference<int>, int&&>);
static_assert(di::conc::SameAs<di::meta::AddRValueReference<int&>, int&>);
static_assert(di::conc::SameAs<di::meta::AddRValueReference<int&&>, int&&>);
static_assert(di::conc::SameAs<di::meta::AddRValueReference<void const>, void const>);

static_assert(di::conc::SameAs<di::meta::AddLValueReference<int>, int&>);
static_assert(di::conc::SameAs<di::meta::AddLValueReference<int&>, int&>);
static_assert(di::conc::SameAs<di::meta::AddLValueReference<int&&>, int&>);
static_assert(di::conc::SameAs<di::meta::AddLValueReference<void const>, void const>);

static_assert(di::conc::SameAs<di::meta::Like<int const&, i32>, i32 const&>);
static_assert(di::conc::SameAs<di::meta::Like<int const&&, i32>, i32 const&&>);
static_assert(di::conc::SameAs<di::meta::Like<int&&, i32>, i32&&>);
static_assert(di::conc::SameAs<di::meta::Like<int&, i32>, i32&>);
static_assert(di::conc::SameAs<di::meta::Like<int, i32>, i32>);
