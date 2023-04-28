#include <locale.h>

extern "C" char* setlocale(int, char const*) {
    // NOTE: This is valid for the "C" locale, but may needed to be changed if other locales are supported.
    return const_cast<char*>("C");
}
