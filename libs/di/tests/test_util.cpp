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

constexpr void uuid() {
    // Standard Endian Format.
    auto x = "00112233-4455-6677-8899-aabbccddeeff"_uuid;
    auto ex = di::Array {
        0x00_b, 0x11_b, 0x22_b, 0x33_b, 0x44_b, 0x55_b, 0x66_b, 0x77_b,
        0x88_b, 0x99_b, 0xaa_b, 0xbb_b, 0xcc_b, 0xdd_b, 0xee_b, 0xff_b,
    };

    // Little Endian Format.
    auto y = "00112233-4455-6677-c899-AABBCCDDEEFF"_uuid;
    auto ey = di::Array {
        0x33_b, 0x22_b, 0x11_b, 0x00_b, 0x55_b, 0x44_b, 0x77_b, 0x66_b,
        0xc8_b, 0x99_b, 0xaa_b, 0xbb_b, 0xcc_b, 0xdd_b, 0xee_b, 0xff_b,
    };

    auto ds = di::to_string(x);
    auto es = di::to_string(y);

    ASSERT_EQ(x, di::UUID(ex));
    ASSERT_EQ(ds, "00112233-4455-6677-8899-aabbccddeeff"_sv);
    ASSERT_EQ(y, di::UUID(ey));
    ASSERT_EQ(es, "00112233-4455-6677-c899-aabbccddeeff"_sv);
    ASSERT_NOT_EQ(x, di::UUID());
    ASSERT_NOT_EQ(y, di::UUID());
    ASSERT(!x.null());
    ASSERT(!y.null());
}

TESTC(util, scope_exit)
TESTC(util, uuid)