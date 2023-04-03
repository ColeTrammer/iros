#include <di/prelude.h>
#include <string.h>

extern "C" size_t strlen(char const* string) {
    return di::to_unsigned(di::distance(di::ZCString(string)));
}
