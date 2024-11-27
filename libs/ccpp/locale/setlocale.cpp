#include <locale.h>

extern "C" auto setlocale(int, char const*) -> char* {
    // NOTE: This is valid for the "C" locale, but may needed to be changed if other locales are supported.
    return const_cast<char*>("C");
}
