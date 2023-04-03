#include <di/prelude.h>
#include <string.h>

extern "C" int strcmp(char const* lhs, char const* rhs) {
    auto a = di::ZCUString((unsigned char const*) lhs);
    auto b = di::ZCUString((unsigned char const*) rhs);
    auto result = di::container::compare(a, b);
    if (result < 0) {
        return -1;
    }
    if (result > 0) {
        return 1;
    }

    return 0;
}
