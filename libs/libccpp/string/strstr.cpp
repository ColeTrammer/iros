#include <ccpp/bits/zstring.h>

extern "C" unsigned char const* strstr(unsigned char const* haystack_str, unsigned char const* needle_str) {
    auto haystack = ccpp::UZString { haystack_str };
    auto needle = ccpp::UZString { needle_str };
    if (di::empty(needle)) {
        return haystack.data();
    }

    auto [first, last] = di::search(haystack, needle);
    if (first == last) {
        return nullptr;
    }
    return first.base();
}