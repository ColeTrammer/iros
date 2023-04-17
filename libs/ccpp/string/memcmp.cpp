#include <di/container/algorithm/prelude.h>
#include <string.h>

extern "C" int memcmp(void const* lhs, void const* rhs, size_t count) {
    auto const* lhs_typed = (unsigned char const*) lhs;
    auto const* rhs_typed = (unsigned char const*) rhs;
    auto result = di::container::compare(lhs_typed, lhs_typed + count, rhs_typed, rhs_typed + count);
    if (result < 0) {
        return -1;
    }
    if (result > 0) {
        return 1;
    }
    return 0;
}
