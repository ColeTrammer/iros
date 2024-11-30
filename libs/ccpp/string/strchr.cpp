#include <string.h>

extern "C" auto strchr(char const* haystack, int needle) -> char* {
    auto needle_typed = (unsigned char) (char) needle;
    auto* haystack_typed = (unsigned char*) haystack;

    if (*haystack_typed == needle_typed) {
        return (char*) haystack_typed;
    }
    if (*haystack_typed == '\0') {
        return nullptr;
    }

    do {
        if (*++haystack_typed == needle_typed) {
            return (char*) haystack_typed;
        }
    } while (*haystack_typed != '\0');
    return nullptr;
}
