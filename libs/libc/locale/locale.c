#include <locale.h>
#include <stdio.h>

char *setlocale(int category, const char *locale) {
    (void) category;
    (void) locale;

    return "C";
}