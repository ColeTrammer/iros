#include <di/math/bigint/prelude.h>
#include <di/util/prelude.h>

__extension__ typedef int raw_i128 __attribute__((mode(TI)));
__extension__ typedef unsigned raw_u128 __attribute__((mode(TI)));

extern "C" [[gnu::weak]] raw_u128 __udivti3(raw_u128 a, raw_u128 b) {
    return di::bit_cast<raw_u128>(di::bit_cast<di::u128_fallback>(a) / di::bit_cast<di::u128_fallback>(b));
}
extern "C" [[gnu::weak]] raw_u128 __umodti3(raw_u128 a, raw_u128 b) {
    return di::bit_cast<raw_u128>(di::bit_cast<di::u128_fallback>(a) % di::bit_cast<di::u128_fallback>(b));
}
extern "C" [[gnu::weak]] raw_i128 __divti3(raw_i128 a, raw_i128 b) {
    return di::bit_cast<raw_i128>(di::bit_cast<di::i128_fallback>(a) / di::bit_cast<di::i128_fallback>(b));
}
