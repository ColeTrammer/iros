#include <string.h>

extern "C" int strcoll(char const* lhs, char const* rhs) {
    // NOTE: This is valid for the "C" locale, but may needed to be changed if other locales are supported.
    return strcmp(lhs, rhs);
}
