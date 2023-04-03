#include <string.h>

extern "C" char* strchr(char const* str, int ch) {
    auto needle = (char) ch;
    auto* str_typed = (unsigned char*) str;

    if (*str_typed == needle) {
        return (char*) str_typed;
    } else if (*str_typed == '\0') {
        return nullptr;
    }

    do {
        if (*++str_typed == needle) {
            return (char*) str_typed;
        }
    } while (*str_typed != '\0');
    return nullptr;
}
