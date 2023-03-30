#include <di/prelude.h>

extern "C" int strncmp(unsigned char const* lhs, unsigned char const* rhs, size_t count) {
    auto a = di::ZCUString(lhs);
    auto b = di::ZCUString(rhs);
    auto result = di::container::compare(di::take(a, count), di::take(b, count));
    if (result < 0) {
        return -1;
    }
    if (result > 0) {
        return 1;
    }
    return 0;
}
