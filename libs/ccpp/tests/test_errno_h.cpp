#ifdef DIUS_USE_RUNTIME
#include <ccpp/bits/all.h>
#else
#include <errno.h>
#endif

#include <dius/test/prelude.h>

namespace errno_h {
static void errno_() {
#if !defined(E2BIG) || !defined(errno) || !defined(EXDEV)
#error "errno and constants are not macros"
#endif

    // Test that errno is thread-local.
    errno = EAGAIN;

    auto thread = *di::create<dius::Thread>([] {
        errno = E2BIG;
        ASSERT_EQ(errno, E2BIG);
    });

    *thread.join();

    ASSERT_EQ(errno, EAGAIN);
}

TEST(errno_h, errno_)
}
