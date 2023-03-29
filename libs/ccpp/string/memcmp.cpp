#include <di/prelude.h>

extern "C" int memcmp(unsigned char* lhs, unsigned char const* rhs, size_t count) {
    auto result = di::container::compare(lhs, lhs + count, rhs, rhs + count);
    if (result < 0) {
        return -1;
    }
    if (result > 0) {
        return 1;
    }
    return 0;
}
