#include <di/prelude.h>
#include <string.h>

extern "C" char* strstr(char const* haystack_str, char const* needle_str) {
    auto haystack = di::ZUString { (unsigned char*) haystack_str };
    auto needle = di::ZCUString { (unsigned char const*) needle_str };
    if (di::empty(needle)) {
        return (char*) haystack.data();
    }

    auto [first, last] = di::search(haystack, needle);
    if (first == last) {
        return nullptr;
    }
    return (char*) first.base();
}
