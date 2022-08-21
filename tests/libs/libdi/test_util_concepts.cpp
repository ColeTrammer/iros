#include <di/util/concepts/conjunction.h>
#include <di/util/concepts/const.h>
#include <di/util/concepts/disjunction.h>
#include <di/util/concepts/floating_point.h>
#include <di/util/concepts/integral.h>
#include <di/util/concepts/language_array.h>
#include <di/util/concepts/member_function_pointer.h>
#include <di/util/concepts/member_object_pointer.h>
#include <di/util/concepts/one_of.h>
#include <di/util/concepts/reference.h>
#include <di/util/prelude.h>

static_assert(di::conc::Reference<i32&&>);
static_assert(di::conc::Const<i32 const>);

static_assert(di::conc::OneOf<i32, i8, i16, i32, i64>);

static_assert(di::conc::LanguageArray<int[32]>);
static_assert(di::conc::LanguageArray<int[]>);
static_assert(!di::conc::LanguageArray<int*>);

struct X {
    int y;
    int z();
};

static_assert(!di::conc::MemberFunctionPointer<decltype(&X::y)>);
static_assert(di::conc::MemberFunctionPointer<decltype(&X::z)>);
static_assert(di::conc::MemberObjectPointer<decltype(&X::y)>);
static_assert(!di::conc::MemberObjectPointer<decltype(&X::z)>);

static_assert(!di::conc::Conjunction<true, false>);
static_assert(di::conc::Conjunction<true>);
static_assert(di::conc::Conjunction<>);

static_assert(di::conc::Disjunction<true, true>);
static_assert(di::conc::Disjunction<true>);
static_assert(!di::conc::Disjunction<>);

static_assert(di::conc::Integral<i32>);
static_assert(di::conc::Integral<unsigned long long volatile const>);

static_assert(di::conc::FloatingPoint<float>);
static_assert(di::conc::FloatingPoint<long double volatile const>);
