#ifdef DIUS_USE_RUNTIME
#include <ccpp/bits/all.h>
#else
#include <stdlib.h>
#endif

#include <dius/test/prelude.h>

namespace stdlib_h {
[[gnu::noinline]] static int do_atoi(char const* s) {
    return atoi(s);
}

static void atoi_() {
    ASSERT_EQ(do_atoi("   -12345HI"), -12345);
    ASSERT_EQ(do_atoi("   +12345HI"), 12345);
    ASSERT_EQ(do_atoi("0"), 0);
    ASSERT_EQ(do_atoi("0042"), 42);
    ASSERT_EQ(do_atoi("0x2A"), 0);
    ASSERT_EQ(do_atoi("HI"), 0);
    ASSERT_EQ(do_atoi("2147483647"), 2147483647);
    ASSERT_EQ(do_atoi("-2147483648"), -2147483648);
}

TEST(stdlib_h, atoi_)
}
