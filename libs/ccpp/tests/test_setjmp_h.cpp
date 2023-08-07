#include <dius/test/prelude.h>
#include <setjmp.h>

namespace setjmp_h {
// Silence clang-tidy warnings about using setjmp and longjmp (for these tests only).
// NOLINTBEGIN cert-err52-cpp
static void longjmp_basic() {
    jmp_buf env;
    auto val = setjmp(env);
    if (val == 0) {
        longjmp(env, 1);
    }
    ASSERT_EQ(val, 1);
}

static void longjmp_value_0() {
    jmp_buf env;
    auto val = setjmp(env);
    if (val == 0) {
        longjmp(env, 0);
    }
    ASSERT_EQ(val, 1);
}

static void longjmp_nested() {
    jmp_buf env1;
    jmp_buf env2;
    auto val1 = setjmp(env1);
    if (val1 == 0) {
        auto val2 = setjmp(env2);
        if (val2 == 0) {
            longjmp(env1, 1);
        }
        ASSERT_EQ(val2, 1);
        longjmp(env2, 2);
    }
    ASSERT_EQ(val1, 1);
}

static void longjmp_nested_functions() {
    static jmp_buf env;

    static auto f1 = [] [[gnu::noinline]] () {
        longjmp(env, 1);
    };

    static auto f2 = [] [[gnu::noinline]] () {
        f1();
    };

    auto val = setjmp(env);
    if (val == 0) {
        f2();
    }

    ASSERT_EQ(val, 1);
}
// NOLINTEND

TEST(errno_h, longjmp_basic)
TEST(errno_h, longjmp_value_0)
TEST(errno_h, longjmp_nested)
TEST(errno_h, longjmp_nested_functions)
}
