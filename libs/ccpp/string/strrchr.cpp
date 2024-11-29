#include <string.h>

extern "C" auto strrchr(char const* str, int ch) -> char* {
    auto needle = (unsigned char) (char) ch;
    auto* str_typed = (unsigned char*) str;

    unsigned char* result = nullptr;
    if (*str_typed == needle) {
        result = str_typed;
    }
    if (*str_typed == '\0') {
        return (char*) result;
    }

    do {
        if (*++str_typed == needle) {
            result = str_typed;
        }
    } while (*str_typed != '\0');
    return (char*) result;
}
