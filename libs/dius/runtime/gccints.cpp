#include <di/math/bigint/prelude.h>
#include <di/util/prelude.h>

__extension__ using raw_i128 = __int128 __attribute__((mode(TI)));
__extension__ using raw_u128 = unsigned __int128 __attribute__((mode(TI)));

extern "C" [[gnu::weak]] auto __udivti3(raw_u128 a, raw_u128 b) -> raw_u128 {
    return di::bit_cast<raw_u128>(di::bit_cast<di::u128_fallback>(a) / di::bit_cast<di::u128_fallback>(b));
}
extern "C" [[gnu::weak]] auto __umodti3(raw_u128 a, raw_u128 b) -> raw_u128 {
    return di::bit_cast<raw_u128>(di::bit_cast<di::u128_fallback>(a) % di::bit_cast<di::u128_fallback>(b));
}
extern "C" [[gnu::weak]] auto __divti3(raw_i128 a, raw_i128 b) -> raw_i128 {
    return di::bit_cast<raw_i128>(di::bit_cast<di::i128_fallback>(a) / di::bit_cast<di::i128_fallback>(b));
}

extern "C" [[gnu::weak]] auto __popcountdi2(i64 value) -> i32 {
    auto c = 0;
    for (auto i = 0_i64; i < 64; ++i) {
        if (value & (1_i64 << i)) {
            ++c;
        }
    }
    return c;
}
