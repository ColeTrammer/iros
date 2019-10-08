#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

int atoi(const char *s) {
	int n = 0;
    size_t i = 0;
    int sign = 1;

    while (isspace(s[i])) { i++; }

    if (s[i] == '+' || s[i] == '-') {
        sign = s[i] == '-' ? -1 : 1;
        i++;
    }

	for (; s[i] != '\0' && isdigit(s[i]); i++) {
		int digit = s[i] - '0';
		n *= 10;
		n += digit;
	}

	return sign * n;
}

long strtol(const char *__restrict str, char **__restrict endptr, int base) {
    /* Should be different on non x86 platforms */
    return (long) strtoll(str, endptr, base);
}

long long strtoll(const char *__restrict str, char **__restrict endptr, int base) {
    if (str == NULL) { return 0; }

    size_t str_off = 0;

    /* Skip initial whitespace */
    while (isspace(str[str_off])) { str_off++; }

    /* Look at optional sign character */
    long long sign = 1;
    if (str[str_off] == '-' || str[str_off] == '+') {
        if (str[str_off] == '-') {
            sign = -1;
        }
        str_off++;
    }

    /* Look at radix prefixes */
    switch (base) {
        case 0: {
            if (str[str_off] == '0' && (str[str_off] == 'x' || str[str_off] == 'X')) {
                base = 16;
                str_off += 2;
            } else if (str[str_off] == '0') {
                base = 8;
                str_off++;
            } else {
                base = 10;
            }
            break;
        }
        case 8:
            if (str[str_off] == '0') { str_off++; }
            break;
        case 16:
            if (str[str_off] == '0' && (str[str_off] == 'x' || str[str_off] == 'X')) { str_off += 2; }
            break;
        default:
            break;
    }

    long long value = 0;

    return sign * value;
}