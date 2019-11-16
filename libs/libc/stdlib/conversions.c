#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

int atoi(const char *s) {
	return (int) atol(s);
}

long atol(const char *s) {
    return (long) atoll(s);
}

long long atoll(const char *s) {
    long long n = 0;
    size_t i = 0;
    long long sign = 1;

    while (isspace(s[i])) { i++; }

    if (s[i] == '+' || s[i] == '-') {
        sign = s[i] == '-' ? -1 : 1;
        i++;
    }

	for (; s[i] != '\0' && isdigit(s[i]); i++) {
		long long digit = s[i] - '0';
		n *= 10;
		n += digit;
	}

	return sign * n;
}

long strtol(const char *__restrict str, char **__restrict endptr, int base) {
    /* Should be different on non x86 platforms */
    return (long) strtoll(str, endptr, base);
}

unsigned long strtoul(const char *__restrict str, char **__restrict endptr, int base) {
    /* Should be different on non x86 platforms */
    return (unsigned long) strtoull(str, endptr, base);
}

/* Determines the validity of any character for the given base (goes from digits to letters) (max base is 36) */
static bool is_valid_char_for_base(char c, int base) {
    if (isdigit(c)) {
        return (c - '0') < base;
    } else if (isalpha(c)) {
        return (tolower(c) - 'a') < (base - 10);
    }

    return false;
}

/* Must be called on a valid character for the given base (so we don't need bounds checks) */
static unsigned long long get_value_from_char(char c) {
    if (isdigit(c)) {
        return c - '0';
    }

    return tolower(c) - 'a' + 10;
}

long long strtoll(const char *__restrict str, char **__restrict endptr, int base) {
    if (str == NULL) { return 0; }

    size_t str_off = 0;

    /* Skip initial whitespace */
    while (isspace(str[str_off])) { str_off++; }

    /* Look at optional sign character */
    long long sign = 1LL;
    if (str[str_off] == '-' || str[str_off] == '+') {
        if (str[str_off] == '-') {
            sign = -1LL;
        }
        str_off++;
    }

    /* Look at radix prefixes */
    switch (base) {
        case 0: {
            if (str[str_off] == '0' && (str[str_off + 1] == 'x' || str[str_off + 1] == 'X')) {
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
            if (str[str_off] == '0' && (str[str_off + 1] == 'x' || str[str_off + 1] == 'X')) { str_off += 2; }
            break;
        default:
            break;
    }

    unsigned long long value = 0;
    long long ret = 0;
    for (; is_valid_char_for_base(str[str_off], base); str_off++) {
        /* Computer value of the digit */
        unsigned long long digit_value = get_value_from_char(str[str_off]);

        /* Detect overflow */
        if (value > ((sign == 1LL ? ((unsigned long long) LLONG_MAX) : (unsigned long long) -LLONG_MIN) - digit_value) / base) {
            /* Read the rest of the characters but ignore them */
            while (is_valid_char_for_base(str[++str_off], base));

            ret = sign == 1LL ? LLONG_MAX : LLONG_MIN;
            errno = ERANGE;
            goto finish;
        }

        /* Compute the value by adding the value and muliplying by radix */
        value *= base;
        value += get_value_from_char(str[str_off]);
    }

    ret = sign * value;

finish:
    if (endptr != NULL) {
        *endptr = (char*) (str + str_off);
    }
    return ret;
}

unsigned long long strtoull(const char *__restrict str, char **__restrict endptr, int base) {
    if (str == NULL) { return 0; }

    size_t str_off = 0;

    /* Skip initial whitespace */
    while (isspace(str[str_off])) { str_off++; }

    /* Look at radix prefixes */
    switch (base) {
        case 0: {
            if (str[str_off] == '0' && (str[str_off + 1] == 'x' || str[str_off + 1] == 'X')) {
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
            if (str[str_off] == '0' && (str[str_off + 1] == 'x' || str[str_off + 1] == 'X')) { str_off += 2; }
            break;
        default:
            break;
    }

    unsigned long long value = 0;
    for (; is_valid_char_for_base(str[str_off], base); str_off++) {
        /* Computer value of the digit */
        unsigned long long digit_value = get_value_from_char(str[str_off]);

        /* Detect overflow */
        if (value > (ULLONG_MAX - digit_value) / base) {
            /* Read the rest of the characters but ignore them */
            while (is_valid_char_for_base(str[++str_off], base));

            value = ULLONG_MAX;
            errno = ERANGE;
            break;
        }

        /* Compute the value by adding the value and muliplying by radix */
        value *= base;
        value += get_value_from_char(str[str_off]);
    }

    if (endptr != NULL) {
        *endptr = (char*) (str + str_off);
    }
    return value;
}

#ifndef __is_libk

double atof(const char *s) {
    return strtod(s, NULL);
}

double strtod(const char *__restrict str, char **__restrict endptr) {
    if (str == NULL) { return 0; }

    size_t str_off = 0;

    /* Skip initial whitespace */
    while (isspace(str[str_off])) { str_off++; }

    /* Look at optional sign character */
    double sign = 1.0;
    if (str[str_off] == '-' || str[str_off] == '+') {
        if (str[str_off] == '-') {
            sign = -1.0;
        }
        str_off++;
    }

    double value = 0.0;
    double decimal_offset = 10.0;
    double ret = 0.0;
    bool encountered_decimal = false;
    for (; isdigit(str[str_off]) || (!encountered_decimal && str[str_off] == '.'); str_off++) {
        if (str[str_off] == '.') {
            encountered_decimal = true;
            continue;
        }

        /* Computer value of the digit */
        int digit_value = str[str_off] - '0';

        /* Compute the value by adding the value and muliplying by radix */
        if (!encountered_decimal) {
            value *= 10.0;
            value += digit_value;
        } else {
            value += ((double) digit_value) / decimal_offset;
            decimal_offset *= 10.0;
        }
    }

    ret = sign * value;

    if (endptr != NULL) {
        *endptr = (char*) (str + str_off);
    }
    return ret;
}

#endif /* __is_libk */