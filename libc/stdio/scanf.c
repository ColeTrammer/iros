#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
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

int scanf_internal(int (*get_character)(void *state), void *__restrict state, const char *__restrict format, va_list parameters) {
    size_t format_off = 0;
    size_t num_read = 0;
    bool done = false;
    char c = '\0';
    while (!done || format[format_off] != '\0') {
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

        struct scanf_specifier_state specifer = { false, INT_MAX, SCANF_LENGTH_DEFAULT, NULL };
        if (format[format_off] == '*') {
            specifer.star = true;
            format_off++;
        }

        if (isdigit(format[format_off])) {
            specifer.width = atoi(format + format_off);
            while (isdigit(format[format_off])) { format_off++; }
        }

        switch(format[format_off]) {
            case 'h': {
                if (format[format_off + 1] == 'h') {
                    specifer.length = SCANF_LENGTH_CHAR;
                    format_off += 2;
                    break;
                } else {
                    specifer.length = SCANF_LENGTH_SHORT;
                    format_off++;
                    break;
                }
            }
            case 'l': {
                if (format[format_off + 1] == 'l') {
                    specifer.length = SCANF_LENGTH_LONG_LONG;
                    format_off += 2;
                    break;
                } else {
                    specifer.length = SCANF_LENGTH_LONG;
                    format_off++;
                    break;
                }
            }
            case 'j':
                specifer.length = SCANF_LENGTH_INTMAX;
                format_off++;
                break;
            case 'z':
                specifer.length = SCANF_LENGTH_SIZE_T;
                format_off++;
                break;
            case 't':
                specifer.length = SCANF_LENGTH_PTRDIFF;
                format_off++;
                break;
            case 'L':
                specifer.length = SCANF_LENGTH_LONG_DOUBLE;
                format_off++;
                break;
            default:
                break;
        }

        specifer.specifier = format + format_off;

        switch(*specifer.specifier) {
            case 'i': {
                /* Should be maximum number of chars ULONG_MAX can be */
                char buffer[30];
                size_t buffer_index = 0;
                buffer[buffer_index++] = c;
                int ret;
                while (buffer_index < 30 && isdigit(ret = get_character(state))) {
                    if (ret == EOF) {
                        done = true;
                        break;
                    }
                    buffer[buffer_index++] = (char) ret;
                }
                if (ret == EOF) {
                    done = true;
                } else {
                    c = (char) ret;
                }
                buffer[buffer_index] = '\0';

                /* Should instead be using atleast atol, maybe even atoll */
                int value = atoi(buffer);

                /* Should look at length field */
                int *place_here = va_arg(parameters, int*);
                *place_here = value;
                num_read++;
                format_off++;
                break;
            }

            /* Currently %f, %e, %g, %a will go here because supporting floats is hard */
            default:
#ifdef __is_libk
                debug_log("Unsupported specifier: %s\n", specifer.specifier);
#else
                fprintf(stderr, "Unsupported specifier: %s\n", specifer.specifier);
#endif /* __is_libk */
                return num_read;
        }
    }

    return num_read;
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