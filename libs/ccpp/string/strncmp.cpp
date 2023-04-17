#include <di/container/algorithm/prelude.h>
#include <di/container/string/prelude.h>
#include <di/container/view/prelude.h>
#include <string.h>

extern "C" int strncmp(char const* lhs, char const* rhs, size_t count) {
    auto a = di::ZCUString((unsigned char const*) lhs);
    auto b = di::ZCUString((unsigned char const*) rhs);
    auto result = di::container::compare(di::take(a, count), di::take(b, count));
    if (result < 0) {
        return -1;
    }
    if (result > 0) {
        return 1;
    }
    return 0;
}
