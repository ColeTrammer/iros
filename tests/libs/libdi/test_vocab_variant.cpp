#include <di/function/index_dispatch.h>
#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    // auto v = di::Variant<int, short, long>(di::in_place_index<1>, 1);

    static_assert(di::SameAs<int, di::meta::At<di::meta::List<char, short, int>, 2>>);

    static_assert(
        di::SameAs<di::meta::List<di::meta::SizeConstant<0>, di::meta::SizeConstant<1>>, di::meta::AsList<di::meta::IndexSequence<0, 1>>>);

    using namespace di::meta;
    static_assert(di::SameAs<List<List<int, float>, List<float, int>>, Zip<List<int, float>, List<float, int>>>);

    static_assert(sizeof(di::Variant<int, short, long, long, long, long, long, long>) == 16);
}

TEST_CONSTEXPR(vocab_variant, basic, basic)
