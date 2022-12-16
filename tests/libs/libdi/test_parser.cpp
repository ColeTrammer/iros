#include <di/prelude.h>
#include <test/test.h>

constexpr void set() {
    auto pred = U'A'_m - U'Z'_m;

    ASSERT(pred(U'B'));
    ASSERT(!pred(U'0'));

    auto alpha_num = 'a'_m - 'z'_m || 'A'_m - 'Z'_m || '0'_m - '9'_m;
    ASSERT(alpha_num(U'3'));
    ASSERT(!alpha_num(U'-'));

    auto not_alpha_num = !alpha_num;
    ASSERT(!not_alpha_num(U'3'));
    ASSERT(not_alpha_num(U'-'));
}

TEST_CONSTEXPR(parser, set, set)