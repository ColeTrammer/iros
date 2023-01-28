#include <di/prelude.h>
#include <dius/test/prelude.h>

constexpr void scope_exit() {
    int value = 5;
    {
        auto guard = di::ScopeExit([&] {
            value = 42;
        });

        auto o = di::move(guard);
    }
    ASSERT_EQ(value, 42);

    {
        auto guard = di::ScopeExit([&] {
            value = 5;
        });
        guard.release();
    }
    ASSERT_EQ(value, 42);
}

TESTC(util, scope_exit)