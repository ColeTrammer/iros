#include <di/prelude.h>

extern "C" int strcmp(unsigned char const* lhs, unsigned char const* rhs) {
    auto a = di::ZCUString(lhs);
    auto b = di::ZCUString(rhs);
    auto result = di::container::compare(a, b);
    if (result < 0) {
        return -1;
    }
    if (result > 0) {
        return 1;
    }

    return 0;
}
