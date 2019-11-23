#include <locale.h>
#include <stdio.h>

static struct lconv current_lconv = {
    "$", ".", '/', "3", "$$$.", '/', 0, 0, 0, 0, 0, 0,
    ".", "3", ",", "-", 0, 0, 0, "+", 0, 0, 0, ","
};

char *setlocale(int category, const char *locale) {
    (void) category;
    (void) locale;

    return "C";
}

struct lconv *localeconv(void) {
    return &current_lconv;
}