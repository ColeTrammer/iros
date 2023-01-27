#include <ccpp/bits/zstring.h>
#include <ccpp/string.h>

extern "C" size_t strlen(char const* string) {
    return di::to_unsigned(di::distance(ccpp::ZString(string)));
}