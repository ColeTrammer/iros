#include <di/container/algorithm/prelude.h>
#include <di/container/string/prelude.h>
#include <string.h>

extern "C" auto strstr(char const* haystack, char const* needle) -> char* {
    auto haystack_zstring = di::ZUString { (unsigned char*) haystack };
    auto needle_zstring = di::ZCUString { (unsigned char const*) needle };
    if (di::empty(needle_zstring)) {
        return (char*) haystack_zstring.data();
    }

    auto [first, last] = di::search(haystack_zstring, needle_zstring);
    if (first == last) {
        return nullptr;
    }
    return (char*) first.base();
}
