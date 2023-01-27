#include <di/prelude.h>
#include <dius/test/prelude.h>

di::Generator<int> f() {
    co_yield 1;
    co_yield 2;
    co_yield 3;
}

void basic() {
    static_assert(di::concepts::InputContainer<di::Generator<int>>);
    static_assert(!di::concepts::ForwardContainer<di::Generator<int>>);

    auto result = f() | di::to<di::Vector>();
    auto ex1 = di::Array { 1, 2, 3 } | di::to<di::Vector>();

    ASSERT_EQ(result, ex1);
    ASSERT(di::container::equal(f(), di::Array { 1, 2, 3 }));
}

TEST(function_generator, basic)