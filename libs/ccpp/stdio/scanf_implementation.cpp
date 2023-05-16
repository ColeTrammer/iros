#include <ccpp/bits/scanf_implementation.h>
#include <ctype.h>
#include <di/assert/prelude.h>
#include <di/math/prelude.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

namespace ccpp {
struct scanf_specifier_state {
    bool star;
    bool memory;
    int width;

#define SCANF_LENGTH_DEFAULT     0
#define SCANF_LENGTH_CHAR        1
#define SCANF_LENGTH_SHORT       2
#define SCANF_LENGTH_LONG        3
#define SCANF_LENGTH_LONG_LONG   4
#define SCANF_LENGTH_INTMAX      5
#define SCANF_LENGTH_SIZE_T      6
#define SCANF_LENGTH_PTRDIFF     7
#define SCANF_LENGTH_LONG_DOUBLE 8
    int length;
    char const* specifier;
};

#define SCANF_NUMBER_BUFFER_MAX 30

static int determine_base(int c) {
    switch (c) {
        case 'i':
            return 0;
        case 'd':
        case 'u':
            return 10;
        case 'o':
            return 8;
        case 'X':
        case 'x':
            return 16;
        default:
            return 10;
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

// Determines whether a character is in a given set
static bool is_valid_char_for_set(char c, char const* set, int set_end, bool invert) {
    for (int i = 0; i < set_end; i++) {
        // Handle `-` ranges
        if (i != 0 && i != set_end - 1 && set[i] == '-') {
            char range_start = set[i - 1];
            char range_end = set[i + 1];

            // Switch ranges so they work correctly if specified backwards
            if (range_start > range_end) {
                char t = range_end;
                range_end = range_start;
                range_start = t;
            }

            // Don't need to check edges b/c they are checked automatically
            for (char r = range_start + 1; r < range_end; r++) {
                if (r == c) {
                    return !invert;
                }
            }
        }

        if (set[i] == c) {
            return !invert;
        }
    }

    return invert;
}

di::Expected<int, di::GenericCode>
scanf_implementation(di::FunctionRef<di::Expected<di::Optional<char>, di::GenericCode>()> read_next, char const* format,
                     va_list args) {
    void* state = nullptr;
    auto get_character = [&](void*) -> int {
        auto ch = read_next();
        if (!ch) {
            return EOF;
        }
        if (!*ch) {
            return EOF;
        }
        return **ch;
    };

    size_t format_off = 0;
    int num_read = 0;
    bool done = false;
    char c = '\0';
    while (!done && format[format_off] != '\0') {
        if (isspace(format[format_off])) {
            int ret;
            while (isspace(ret = get_character(state)))
                ;
            if (ret == EOF) {
                return num_read;
            }
            c = (char) ret;
            format_off++;
        } else if (c == '\0') {
            int ret = get_character(state);
            if (ret == EOF) {
                return num_read;
            }
            c = (char) ret;
        }

        if (format[format_off] != '%' || (format[format_off] == '%' && format[format_off + 1] == '%')) {
            if (format[format_off] == '%') {
                format_off++;
            }

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

        struct scanf_specifier_state specifier = { false, false, di::NumericLimits<int>::max, SCANF_LENGTH_DEFAULT,
                                                   NULL };
        if (format[format_off] == '*') {
            specifier.star = true;
            format_off++;
        }

        if (format[format_off] == 'm') {
            specifier.memory = true;
            ASSERT(false);
            format_off++;
        }

        if (isdigit(format[format_off])) {
            specifier.width = atoi(format + format_off);
            while (isdigit(format[format_off])) {
                format_off++;
            }
        }

        switch (format[format_off]) {
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

        switch (*specifier.specifier) {
            case 'c': {
                if (specifier.width == di::NumericLimits<int>::max) {
                    specifier.width = 1;
                }
                int i = 0;
                int ret = 0;

                char* buf = NULL;
                if (!specifier.star) {
                    buf = va_arg(args, char*);
                    buf[i] = c;
                }

                i++;

                while (i < specifier.width) {
                    /* Should probably fail if we can't read in exactly the right amount */
                    ret = get_character(state);
                    if (ret == EOF) {
                        goto finish;
                    }

                    if (!specifier.star) {
                        buf[i] = c;
                    }

                    i++;
                }

                if (!specifier.star) {
                    num_read++;
                }
                format_off++;
                c = '\0';
                break;
            }

            case 's': {
                int ret = 0;
                /* Ignore initial whitespace */
                while (isspace(c)) {
                    ret = get_character(state);
                    if (ret == EOF) {
                        goto finish;
                    }
                    c = (char) ret;
                }

                int i = 0;
                char* buf = NULL;
                if (!specifier.star) {
                    buf = va_arg(args, char*);
                    buf[i] = c;
                }

                i++;

                while (i < specifier.width && (!isspace(ret = get_character(state)) && ret != EOF)) {
                    if (!specifier.star) {
                        buf[i] = (char) ret;
                    }

                    i++;
                }

                if (!specifier.star) {
                    buf[i] = '\0';
                }

                if (ret != EOF && i < specifier.width) {
                    c = (int) ret;
                } else if (ret == EOF) {
                    done = true;
                } else {
                    c = '\0';
                }

                if (!specifier.star) {
                    num_read++;
                }

                format_off++;
                break;
            }

            case '[': {
                format_off++;

                bool invert = false;
                if (format[format_off] == '^') {
                    invert = true;
                    format_off++;
                }

                // Always assume at least 1 character in set (there has to be and this allows for inclusion of `]`)
                int set_start = format_off;
                int set_end = set_start + 1;
                while (format[set_end] != ']') {
                    set_end++;
                }

                if (!is_valid_char_for_set(c, format + format_off, set_end - set_start, invert)) {
                    goto finish;
                }

                int ret = 0;
                int i = 0;
                char* buf = NULL;
                if (!specifier.star) {
                    buf = va_arg(args, char*);
                    buf[i] = c;
                }

                i++;

                while (i < specifier.width && (is_valid_char_for_set(ret = get_character(state), format + format_off,
                                                                     set_end - set_start, invert) &&
                                               ret != EOF)) {
                    if (!specifier.star) {
                        buf[i] = (char) ret;
                    }

                    i++;
                }

                if (!specifier.star) {
                    buf[i] = '\0';
                }

                if (ret != EOF && i < specifier.width) {
                    c = (int) ret;
                } else if (ret == EOF) {
                    done = true;
                } else {
                    c = '\0';
                }

                if (!specifier.star) {
                    num_read++;
                }

                format_off = set_end + 1;
                break;
            }

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

                if (!isdigit(c) && c != '+' && c != '-') {
                    goto finish;
                }

                buffer[buffer_index++] = c;

                /* Copy str character by character into buffer */
                while (buffer_index < specifier.width && buffer_index < SCANF_NUMBER_BUFFER_MAX - 1 &&
                       (isdigit(ret = get_character(state)))) {
                    buffer[buffer_index++] = (char) ret;
                }
                if (ret != EOF && buffer_index < specifier.width) {
                    c = (char) ret;
                } else if (ret == EOF) {
                    done = true;
                } else {
                    c = '\0';
                }
                buffer[buffer_index] = '\0';

                /* Read in the largest value and cast it down later since we don't care about overflows in this function
                 */
                long long value = strtoll(buffer, NULL, determine_base(*specifier.specifier));

                /* Don't save it if there is was a `*` */
                if (specifier.star) {
                    format_off++;
                    break;
                }

                /* Concert to right length */
                switch (specifier.length) {
                    case SCANF_LENGTH_CHAR: {
                        char* place_here = va_arg(args, char*);
                        *place_here = (char) value;
                        break;
                    }
                    case SCANF_LENGTH_SHORT: {
                        short* place_here = va_arg(args, short*);
                        *place_here = (short) value;
                        break;
                    }
                    case SCANF_LENGTH_LONG: {
                        long* place_here = va_arg(args, long*);
                        *place_here = (long) value;
                        break;
                    }
                    case SCANF_LENGTH_LONG_LONG: {
                        long long* place_here = va_arg(args, long long*);
                        *place_here = (long long) value;
                        break;
                    }
                    case SCANF_LENGTH_INTMAX: {
                        intmax_t* place_here = va_arg(args, intmax_t*);
                        *place_here = (intmax_t) value;
                        break;
                    }
                    case SCANF_LENGTH_SIZE_T: {
                        size_t* place_here = va_arg(args, size_t*);
                        *place_here = (size_t) value;
                        break;
                    }
                    case SCANF_LENGTH_PTRDIFF: {
                        ptrdiff_t* place_here = va_arg(args, ptrdiff_t*);
                        *place_here = (ptrdiff_t) value;
                        break;
                    }
                    default: {
                        int* place_here = va_arg(args, int*);
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

                int base = determine_base(*specifier.specifier);

                /* Should be maximum number of chars ULONG_MAX can be */
                char buffer[SCANF_NUMBER_BUFFER_MAX];
                int buffer_index = 0;

                if (!is_valid_char_for_base(c, base) && c != '-' && c != '+') {
                    goto finish;
                }

                buffer[buffer_index++] = c;

                /* Copy str character by character into buffer */
                while (buffer_index < specifier.width && buffer_index < SCANF_NUMBER_BUFFER_MAX - 1 &&
                       (is_valid_char_for_base(ret = get_character(state), base) ||
                        (base == 16 && buffer_index == 1 && (ret == 'x' || ret == 'X')))) {
                    if (ret == EOF) {
                        done = true;
                        break;
                    }
                    buffer[buffer_index++] = (char) ret;
                }
                if (ret != EOF && buffer_index < specifier.width) {
                    c = (char) ret;
                } else {
                    c = '\0';
                }
                buffer[buffer_index] = '\0';

                /* Read in the largest value and cast it down later since we don't care about overflows in this function
                 */
                unsigned long long value = strtoull(buffer, NULL, base);

                /* Don't save it if there is was a `*` */
                if (specifier.star) {
                    format_off++;
                    break;
                }

                /* Concert to right length */
                switch (specifier.length) {
                    case SCANF_LENGTH_CHAR: {
                        unsigned char* place_here = va_arg(args, unsigned char*);
                        *place_here = (unsigned char) value;
                        break;
                    }
                    case SCANF_LENGTH_SHORT: {
                        unsigned short* place_here = va_arg(args, unsigned short*);
                        *place_here = (unsigned short) value;
                        break;
                    }
                    case SCANF_LENGTH_LONG: {
                        unsigned long* place_here = va_arg(args, unsigned long*);
                        *place_here = (unsigned long) value;
                        break;
                    }
                    case SCANF_LENGTH_LONG_LONG: {
                        unsigned long long* place_here = va_arg(args, unsigned long long*);
                        *place_here = (unsigned long long) value;
                        break;
                    }
                    case SCANF_LENGTH_INTMAX: {
                        uintmax_t* place_here = va_arg(args, uintmax_t*);
                        *place_here = (uintmax_t) value;
                        break;
                    }
                    case SCANF_LENGTH_SIZE_T: {
                        size_t* place_here = va_arg(args, size_t*);
                        *place_here = (size_t) value;
                        break;
                    }
                    case SCANF_LENGTH_PTRDIFF: {
                        ptrdiff_t* place_here = va_arg(args, ptrdiff_t*);
                        *place_here = (ptrdiff_t) value;
                        break;
                    }
                    default: {
                        unsigned int* place_here = va_arg(args, unsigned int*);
                        *place_here = (unsigned int) value;
                        break;
                    }
                }

                num_read++;
                format_off++;
                break;
            }

            /* Floating point */
#undef __SSE__
#ifdef __SSE__
            case 'a':
            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G': {
                int ret = 0;

                /* Ignore initial whitespace */
                while (isspace(c)) {
                    ret = get_character(state);
                    if (ret == EOF) {
                        goto finish;
                    }
                    c = (char) ret;
                }

                /* Should be maximum number of chars a long double can be */
                char buffer[SCANF_NUMBER_BUFFER_MAX];
                int buffer_index = 0;

                if (!is_valid_char_for_base(c, 16) && c != '.' && c != '-' && c != '+') {
                    goto finish;
                }

                buffer[buffer_index++] = c;

                /* Copy str character by character into buffer */
                bool seen_decimal = c == '.';
                while (buffer_index < specifier.width && buffer_index < SCANF_NUMBER_BUFFER_MAX - 1 &&
                       (is_valid_char_for_base(ret = get_character(state), 16) || (!seen_decimal && ret == '.') ||
                        (buffer_index == 1 && (ret == 'x' || ret == 'X')))) {
                    if (ret == EOF) {
                        done = true;
                        break;
                    }
                    buffer[buffer_index++] = (char) ret;
                    seen_decimal = ret == '.';
                }
                if (ret != EOF && buffer_index < specifier.width) {
                    c = (char) ret;
                } else {
                    c = '\0';
                }
                buffer[buffer_index] = '\0';

                /* Read in the largest value and cast it down later since we don't care about overflows in this function
                 */
                long double value = strtold(buffer, NULL);

                /* Don't save it if there is was a `*` */
                if (specifier.star) {
                    format_off++;
                    break;
                }

                /* Convert to right length */
                switch (specifier.length) {
                    case SCANF_LENGTH_LONG: {
                        double* place_here = va_arg(args, double*);
                        *place_here = (double) value;
                        break;
                    }
                    case SCANF_LENGTH_LONG_DOUBLE: {
                        long double* place_here = va_arg(args, long double*);
                        *place_here = (long double) value;
                        break;
                    }
                    default: {
                        float* place_here = va_arg(args, float*);
                        *place_here = (float) value;
                        break;
                    }
                }

                num_read++;
                format_off++;
                break;
            }
#endif /* __SSE__ */

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
}
