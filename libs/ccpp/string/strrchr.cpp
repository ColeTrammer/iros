#include <string.h>

extern "C" auto strrchr(char const* haystack, int needle) -> char* {
    auto needle_typed = (unsigned char) (char) needle;
    auto* haystack_typed = (unsigned char*) haystack;

    unsigned char* result = nullptr;
    if (*haystack_typed == needle_typed) {
        result = haystack_typed;
    }
    if (*haystack_typed == '\0') {
        return (char*) result;
    }

    do {
        if (*++haystack_typed == needle_typed) {
            result = haystack_typed;
        }
    } while (*haystack_typed != '\0');
    return (char*) result;
}
