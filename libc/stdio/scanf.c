#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#ifdef __is_libk
#include <kernel/hal/output.h>
#endif /* __is_libk */

struct scanf_specifier_state {
    bool star;
    int width;

#define SCANF_LENGTH_DEFAULT 0
#define SCANF_LENGTH_CHAR 1
#define SCANF_LENGTH_SHORT 2
#define SCANF_LENGTH_LONG 3
#define SCANF_LENGTH_LONG_LONG 4
#define SCANF_LENGTH_INTMAX 5
#define SCANF_LENGTH_SIZE_T 6
#define SCANF_LENGTH_PTRDIFF 7
#define SCANF_LENGTH_LONG_DOUBLE 8
    int length;
    const char *specifier;
};

#define SCANF_NUMBER_BUFFER_MAX 30

static int determine_base(int c) {
    switch (c) {
        case 'i': return 0;
        case 'd':
        case 'u': return 10;
        case 'o': return 8;
        case 'X':
        case 'x': return 16;
        default : return 10; 
    }
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

int scanf_internal(int (*get_character)(void *state), void *__restrict state, const char *__restrict format, va_list parameters) {
    size_t format_off = 0;
    int num_read = 0;
    bool done = false;
    char c = '\0';
    while (!done && format[format_off] != '\0') {
        if (isspace(format[format_off])) {
            int ret;
            while (isspace(ret = get_character(state)));
            if (ret == EOF) {
                return num_read;
            }
            c = (char) ret;
        } else if (c == '\0') {
            int ret = get_character(state);
            if (ret == EOF) {
                return num_read;
            } 
            c = (char) ret;
        }

        if (format[format_off] != '%' || (format[format_off] == '%' && format[format_off + 1] == '%')) {
            if (format[format_off] == '%') { format_off++; }

            if (c != format[format_off]) {
                return num_read;
            }

            /* Tell code it needs to read the next character */
            c = '\0';
            format_off++;
            continue;
        }

        /* Guanteed to be the beginning of a format specifier */
        format_off++;

        struct scanf_specifier_state specifier = { false, INT_MAX, SCANF_LENGTH_DEFAULT, NULL };
        if (format[format_off] == '*') {
            specifier.star = true;
            format_off++;
        }

        if (isdigit(format[format_off])) {
            specifier.width = atoi(format + format_off);
            while (isdigit(format[format_off])) { format_off++; }
        }

        switch(format[format_off]) {
            case 'h': {
                if (format[format_off + 1] == 'h') {
                    specifier.length = SCANF_LENGTH_CHAR;
                    format_off += 2;
                    break;
                } else {
                    specifier.length = SCANF_LENGTH_SHORT;
                    format_off++;
                    break;
                }
            }
            case 'l': {
                if (format[format_off + 1] == 'l') {
                    specifier.length = SCANF_LENGTH_LONG_LONG;
                    format_off += 2;
                    break;
                } else {
                    specifier.length = SCANF_LENGTH_LONG;
                    format_off++;
                    break;
                }
            }
            case 'j':
                specifier.length = SCANF_LENGTH_INTMAX;
                format_off++;
                break;
            case 'z':
                specifier.length = SCANF_LENGTH_SIZE_T;
                format_off++;
                break;
            case 't':
                specifier.length = SCANF_LENGTH_PTRDIFF;
                format_off++;
                break;
            case 'L':
                specifier.length = SCANF_LENGTH_LONG_DOUBLE;
                format_off++;
                break;
            default:
                break;
        }

        specifier.specifier = format + format_off;

        switch(*specifier.specifier) {
            /* Signed integers */
            case 'd':
            case 'i': {
                int ret = 0;
                /* Ignore initial whitespace */
                while (isspace(c)) {
                    ret = get_character(state);
                    if (ret == EOF) {
                        goto finish;
                    }
                    c = (char) ret;
                }

                /* Should be maximum number of chars ULONG_MAX can be */
                char buffer[SCANF_NUMBER_BUFFER_MAX];
                int buffer_index = 0;
                buffer[buffer_index++] = c;
                /* Copy str character by character into buffer */
                while (buffer_index < specifier.width && buffer_index < SCANF_NUMBER_BUFFER_MAX - 1 && isdigit(ret = get_character(state))) {
                    if (ret == EOF) {
                        done = true;
                        break;
                    }
                    buffer[buffer_index++] = (char) ret;
                }
                if (ret != EOF) {
                    c = (char) ret;
                }
                buffer[buffer_index] = '\0';

                /* Read in the largest value and cast it down later since we don't care about overflows in this function */
                long long value = strtoll(buffer, NULL, determine_base(*specifier.specifier));

                /* Don't save it if there is was a `*` */
                if (specifier.star) {
                    format_off++;
                    break;
                }

                /* Should look at length field */
                switch (specifier.length) {
                    case SCANF_LENGTH_CHAR: {
                        char *place_here = va_arg(parameters, char*);
                        *place_here = (char) value;
                        break;
                    }
                    case SCANF_LENGTH_SHORT: {
                        short *place_here = va_arg(parameters, short*);
                        *place_here = (short) value;
                        break;
                    }
                    case SCANF_LENGTH_LONG: {
                        long *place_here = va_arg(parameters, long*);
                        *place_here = (long) value;
                        break;
                    }
                    case SCANF_LENGTH_LONG_LONG: {
                        long long *place_here = va_arg(parameters, long long*);
                        *place_here = (long long) value;
                        break;
                    }
                    case SCANF_LENGTH_INTMAX: {
                        intmax_t *place_here = va_arg(parameters, intmax_t*);
                        *place_here = (intmax_t) value;
                        break;
                    }
                    case SCANF_LENGTH_SIZE_T: {
                        size_t *place_here = va_arg(parameters, size_t*);
                        *place_here = (size_t) value;
                        break;
                    }
                    case SCANF_LENGTH_PTRDIFF: {
                        ptrdiff_t *place_here = va_arg(parameters, ptrdiff_t*);
                        *place_here = (ptrdiff_t) value;
                        break;
                    }
                    default: {
                        int *place_here = va_arg(parameters, int*);
                        *place_here = (int) value;
                        break;
                    }
                }

                num_read++;
                format_off++;
                break;
            }

            /* Unsigned integers */
            case 'u':
            case 'o':
            case 'x': {
                int ret = 0;

                /* Ignore initial whitespace */
                while (isspace(c)) {
                    ret = get_character(state);
                    if (ret == EOF) {
                        goto finish;
                    }
                    c = (char) ret;
                }

                /* Should be maximum number of chars ULONG_MAX can be */
                int base = determine_base(*specifier.specifier);
                char buffer[SCANF_NUMBER_BUFFER_MAX];
                int buffer_index = 0;
                buffer[buffer_index++] = c;

                /* Copy str character by character into buffer */
                while (buffer_index < specifier.width && buffer_index < SCANF_NUMBER_BUFFER_MAX - 1 && (is_valid_char_for_base(ret = get_character(state), base) || (base == 16 && buffer_index == 1 && (ret == 'x' || ret == 'X')))) {
                    if (ret == EOF) {
                        done = true;
                        break;
                    }
                    buffer[buffer_index++] = (char) ret;
                }
                if (ret != EOF) {
                    c = (char) ret;
                }
                buffer[buffer_index] = '\0';

                /* Read in the largest value and cast it down later since we don't care about overflows in this function */
                unsigned long long value = strtoull(buffer, NULL, base);

                /* Don't save it if there is was a `*` */
                if (specifier.star) {
                    format_off++;
                    break;
                }

                /* Should look at length field */
                switch (specifier.length) {
                    case SCANF_LENGTH_CHAR: {
                        unsigned char *place_here = va_arg(parameters, unsigned char*);
                        *place_here = (unsigned char) value;
                        break;
                    }
                    case SCANF_LENGTH_SHORT: {
                        unsigned short *place_here = va_arg(parameters, unsigned short*);
                        *place_here = (unsigned short) value;
                        break;
                    }
                    case SCANF_LENGTH_LONG: {
                        unsigned long *place_here = va_arg(parameters, unsigned long*);
                        *place_here = (unsigned long) value;
                        break;
                    }
                    case SCANF_LENGTH_LONG_LONG: {
                        unsigned long long *place_here = va_arg(parameters, unsigned long long*);
                        *place_here = (unsigned long long) value;
                        break;
                    }
                    case SCANF_LENGTH_INTMAX: {
                        uintmax_t *place_here = va_arg(parameters, uintmax_t*);
                        *place_here = (uintmax_t) value;
                        break;
                    }
                    case SCANF_LENGTH_SIZE_T: {
                        size_t *place_here = va_arg(parameters, size_t*);
                        *place_here = (size_t) value;
                        break;
                    }
                    case SCANF_LENGTH_PTRDIFF: {
                        ptrdiff_t *place_here = va_arg(parameters, ptrdiff_t*);
                        *place_here = (ptrdiff_t) value;
                        break;
                    }
                    default: {
                        unsigned int *place_here = va_arg(parameters, unsigned int*);
                        *place_here = (unsigned int) value;
                        break;
                    }
                }

                num_read++;
                format_off++;
                break;
            }

            /* Currently %f, %e, %g, %a will go here because supporting floats is hard */
            default:
#ifdef __is_libk
                debug_log("Unsupported specifier: %s\n", specifier.specifier);
#else
                fprintf(stderr, "Unsupported specifier: %s\n", specifier.specifier);
#endif /* __is_libk */
                return num_read;
        }
    }

finish:
    return num_read == 0 ? EOF : num_read;
}

struct string_scanf_state {
    const char *str;
    size_t i;
};

#ifndef __is_libk

static int file_get_character(void *file) {
    return fgetc(file);
}

#endif /* __is_libk */

static int string_get_character(void *_state) {
    struct string_scanf_state *state = _state;
    int c = state->str[state->i++];
    if (c == '\0') {
        return EOF;
    }

    return c;
}

#ifndef __is_libk

int scanf(const char *__restrict format, ...) {
    va_list parameters;
	va_start(parameters, format);

	int ret = vfscanf(stdin, format, parameters);

	va_end(parameters);
	return ret;
}

int fscanf(FILE *__restrict stream, const char *__restrict format, ...) {
    va_list parameters;
	va_start(parameters, format);

	int ret = vfscanf(stream, format, parameters);

	va_end(parameters);
	return ret;
}

#endif /* __is_libk */

int sscanf(const char *__restrict str, const char *__restrict format, ...) {
    va_list parameters;
	va_start(parameters, format);

	int ret = vsscanf(str, format, parameters);

	va_end(parameters);
	return ret;
}

#ifndef __is_libk

int vscanf(const char *__restrict format, va_list parameters) {
    return vfscanf(stdin, format, parameters);
}

int vfscanf(FILE *stream, const char *__restrict format, va_list parameters) {
    return scanf_internal(&file_get_character, stream, format, parameters);
}

#endif /* __is_libk */

int vsscanf(const char *__restrict str, const char *__restrict format, va_list paramaters) {
    struct string_scanf_state state = { str, 0 };
    return scanf_internal(&string_get_character, &state, format, paramaters);
}