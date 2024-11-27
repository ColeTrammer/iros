#include <di/container/algorithm/prelude.h>
#include <di/container/string/prelude.h>
#include <di/math/prelude.h>
#include <string.h>

extern "C" auto strlen(char const* string) -> size_t {
    return di::to_unsigned(di::distance(di::ZCString(string)));
}
