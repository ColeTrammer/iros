#include <ccpp/bits/printf_implementation.h>
#include <di/container/algorithm/prelude.h>
#include <di/math/prelude.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

namespace ccpp {
static int parseInt(char const* num, size_t length) {
    int n = 0;
    for (size_t i = 0; i < length; i++) {
        int digit = num[i] - '0';
        for (unsigned char t = 1; t < length - i; t++) {
            digit *= 10;
        }
        n += digit;
    }
    return n;
}

di::Expected<int, di::GenericCode>
printf_implementation(di::FunctionRef<di::Expected<void, di::GenericCode>(di::TransparentStringView)> write_exactly,
                      char const* format, va_list args) {
    int written = 0;

    void* obj = nullptr;
    auto print = [&](void* obj, char const* s, usize len) -> bool {
        (void) obj;
        return write_exactly({ s, len }).has_value();
    };

    while (*format != '\0') {
        size_t maxrem = di::NumericLimits<int>::max - written;

        if (format[0] != '%' || format[1] == '%') {
            if (format[0] == '%') {
                format++;
            }
            size_t amount = 1;
            while (format[amount] && format[amount] != '%') {
                amount++;
            }
            if (maxrem < amount) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (!print(obj, format, amount)) {
                return -1;
            }
            format += amount;
            written += amount;
            continue;
        }

        char const* format_begun_at = format++;

        // bitmask with format 000[-][+][ ][#][0]
        unsigned char flags = 0;
        while (*format == '-' || *format == '+' || *format == ' ' || *format == '#' || *format == '0') {
            if (*format == '-') {
                flags |= 0b00010000;
            } else if (*format == '+') {
                flags |= 0b00001000;
            } else if (*format == ' ') {
                flags |= 0b00000100;
            } else if (*format == '#') {
                flags |= 0b00000010;
            } else {
                flags |= 0b00000001;
            }
            format++;
        }

        int width;
        if (*format == '*') {
            width = va_arg(args, int);
            format++;
        } else {
            size_t width_len = 0;
            while (*(format + width_len) >= '0' && *(format + width_len) <= '9') {
                width_len++;
            }
            width = parseInt(format, width_len);
            format += width_len;
        }

        int precision = -1;
        if (*format == '.') {
            format++;
            if (*format == '*') {
                precision = va_arg(args, int);
                format++;
            } else {
                size_t prec_len = 0;
                while (*(format + prec_len) >= '0' && *(format + prec_len) <= '9') {
                    prec_len++;
                }
                precision = parseInt(format, prec_len);
                format += prec_len;
            }
        }
        /* (none) - 0
                h     - 1
    hh    - 2
                l     - 3
                ll    - 6
                j     - 4
                z     - 5
                t     - 7
                L     - 8
        */
        unsigned char length_modifier = 0;
        while (*format == 'h') {
            format++;
            length_modifier++;
        }
        while (*format == 'l') {
            format++;
            length_modifier += 3;
        }
        if (*format == 'j') {
            format++;
            length_modifier = 4;
        } else if (*format == 'z') {
            format++;
            length_modifier = 5;
        } else if (*format == 't') {
            format++;
            length_modifier = 7;
        } else if (*format == 'L') {
            format++;
            length_modifier = 8;
        }

        if (*format == 'c') {
            format++;
            char c = (char) va_arg(args, int /* char promotes to int */);
            if (width < 1) {
                width = 1;
            }
            if (maxrem < (unsigned int) width) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            char space = ' ';
            if (!(flags & 0b00010000)) {
                for (int i = 0; i < width - 1; i++) {
                    if (!print(obj, &space, 1)) {
                        return -1;
                    }
                }
            }
            if (!print(obj, &c, 1)) {
                return -1;
            }
            if (flags & 0b00010000) {
                for (int i = 1; i < width; i++) {
                    if (!print(obj, &space, 1)) {
                        return -1;
                    }
                }
            }
            written += width;
        } else if (*format == 's') {
            char const* str = nullptr;
            if (*format == 's') {
                str = va_arg(args, char const*);
            }

            if (!str) {
                str = "(null)";
            }

            format++;
            size_t len = strlen(str);
            if (len > (unsigned int) width) {
                width = len;
            }
            if (precision >= 0 && precision < width) {
                width = precision;
            }
            if (precision >= 0 && (unsigned int) precision < len) {
                len = width = precision;
            }
            if (maxrem < (unsigned int) width) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (len < (unsigned int) width) {
                char space = ' ';
                if (flags & 0b00010000) {
                    if (!print(obj, str, len)) {
                        return -1;
                    }
                    while (len++ < (unsigned int) width) {
                        if (!print(obj, &space, 1)) {
                            return -1;
                        }
                    }
                } else {
                    for (size_t i = 0; i < width - len; i++) {
                        if (!print(obj, &space, 1)) {
                            return -1;
                        }
                    }
                    if (!print(obj, str, len)) {
                        return -1;
                    }
                }
            } else {
                if (!print(obj, str, len)) {
                    return -1;
                }
            }
            written += width;
        } else if (*format == 'o') {
            format++;
            unsigned int num = va_arg(args, unsigned int);
            size_t len = 1;
            size_t len_prec;
            size_t len_width;
            unsigned int div = 1;
            while (num / div > 7) {
                div *= 8;
                len++;
            }
            if (precision == 0 && num == 0) {
                len = 0;
            }
            len_prec = len;
            len_width = len_prec;
            if (precision >= 0 && (unsigned int) precision > len) {
                len_prec = precision;
                len_width = precision;
            }
            if ((flags & 0b00001000) | (flags & 0b00000100)) {
                len_width++;
            }
            if (flags & 0b00000010) {
                len_width += 2;
            }
            if ((unsigned int) width < len_width) {
                width = len_width;
            }
            if (maxrem < (unsigned int) width) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            char filler = ' ';
            if (!((flags & 0b00010000) | (flags & 0b00000001))) {
                for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                    if (!print(obj, &filler, 1)) {
                        return -1;
                    }
                }
            }
            if (flags & 0b00001000) {
                char plus = '+';
                if (!print(obj, &plus, 1)) {
                    return -1;
                }
            } else if (flags & 0b00000100) {
                char space = ' ';
                if (!print(obj, &space, 1)) {
                    return -1;
                }
            }
            if (flags & 0b00000010) {
                char zero = '0';
                if (!print(obj, &zero, 1)) {
                    return -1;
                }
                if (!print(obj, format, 1)) {
                    return -1;
                }
            }
            if (!(flags & 0b00010000) && (flags & 0b00000001)) {
                filler = '0';
                for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                    if (!print(obj, &filler, 1)) {
                        return -1;
                    }
                }
            }
            while (len < len_prec) {
                char zero = '0';
                if (!print(obj, &zero, 1)) {
                    return -1;
                }
                len++;
            }
            if (num == 0) {
                if (precision != 0) {
                    char zero = '0';
                    if (!print(obj, &zero, 1)) {
                        return -1;
                    }
                }
            } else {
                char digit;
                while (div > 7) {
                    unsigned int n = num / div;
                    div /= 8;
                    digit = n % 8 + '0';
                    if (!print(obj, &digit, 1)) {
                        return -1;
                    }
                }
                digit = num % 8 + '0';
                if (!print(obj, &digit, 1)) {
                    return -1;
                }
            }
            if (flags & 0b00010000) {
                for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                    if (!print(obj, &filler, 1)) {
                        return -1;
                    }
                }
            }
            written += width;
        } else if (*format == 'x' || *format == 'X' || *format == 'p') {
#ifdef __x86_64__
            if (length_modifier == 0 && *format != 'p') {
#else
            if (length_modifier == 0 || *format == 'p' || length_modifier == 3 || length_modifier == 5) {
#endif
                unsigned int num = va_arg(args, unsigned int);
                size_t len = 1;
                size_t len_prec;
                size_t len_width;
                char base_char = *format == 'p' ? 'X' : *format;
                if (*format == 'p') {
                    precision = 8;
                    flags |= 0b10;
                }

                unsigned int div = 1;
                while (num / div > 15) {
                    div *= 16;
                    len++;
                }
                if (precision == 0 && num == 0) {
                    len = 0;
                }
                len_prec = len;
                len_width = len_prec;
                if (precision >= 0 && (unsigned int) precision > len) {
                    len_prec = precision;
                    len_width = precision;
                }
                if ((flags & 0b00001000) | (flags & 0b00000100)) {
                    len_width++;
                }
                if (flags & 0b00000010) {
                    len_width += 2;
                }
                if ((unsigned int) width < len_width) {
                    width = len_width;
                }
                if (maxrem < (unsigned int) width) {
                    // TODO: Set errno to EOVERFLOW.
                    return -1;
                }
                char filler = ' ';
                if (!((flags & 0b00010000) | (flags & 0b00000001))) {
                    for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                        if (!print(obj, &filler, 1)) {
                            return -1;
                        }
                    }
                }
                if (flags & 0b00001000) {
                    char plus = '+';
                    if (!print(obj, &plus, 1)) {
                        return -1;
                    }
                } else if (flags & 0b00000100) {
                    char space = ' ';
                    if (!print(obj, &space, 1)) {
                        return -1;
                    }
                }
                if (flags & 0b00000010) {
                    char zero = '0';
                    if (!print(obj, &zero, 1)) {
                        return -1;
                    }
                    if (!print(obj, &base_char, 1)) {
                        return -1;
                    }
                }
                if (!(flags & 0b00010000) && (flags & 0b00000001)) {
                    filler = '0';
                    for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                        if (!print(obj, &filler, 1)) {
                            return -1;
                        }
                    }
                }
                while (len < len_prec) {
                    char zero = '0';
                    if (!print(obj, &zero, 1)) {
                        return -1;
                    }
                    len++;
                }
                if (num == 0) {
                    if (precision != 0) {
                        char zero = '0';
                        if (!print(obj, &zero, 1)) {
                            return -1;
                        }
                    }
                } else {
                    char digit;
                    while (div > 15) {
                        unsigned int n = num / div;
                        div /= 16;
                        digit = n % 16;
                        if (digit < 10) {
                            digit += '0';
                        } else {
                            digit += base_char - ('x' - 'a') - 10;
                        }
                        if (!print(obj, &digit, 1)) {
                            return -1;
                        }
                    }
                    digit = num % 16;
                    if (digit < 10) {
                        digit += '0';
                    } else {
                        digit += base_char - ('x' - 'a') - 10;
                    }
                    if (!print(obj, &digit, 1)) {
                        return -1;
                    }
                }
                if (flags & 0b00010000) {
                    for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                        if (!print(obj, &filler, 1)) {
                            return -1;
                        }
                    }
                }
                written += width;
                format++;
#if !defined(__is_loader) || !defined(__i386__)
#ifdef __x86_64__
            } else if (length_modifier == 3 || length_modifier == 6 || length_modifier == 5 || *format == 'p') {
#else
            } else if (length_modifier == 6) {
#endif
                uint64_t num = va_arg(args, uint64_t);
                if (num == 0 && *format == 'p') {
                    if (!print(obj, "(null)", 6)) {
                        return -1;
                    }

                    written += 6;
                    format++;
                    continue;
                }

                size_t len = 1;
                size_t len_prec;
                size_t len_width;
                char base_char = *format == 'p' ? 'X' : *format;
                if (*format == 'p') {
                    precision = 16;
                    flags |= 0b10;
                }
                uint64_t div = 1;
                while (num / div > 15) {
                    div *= 16;
                    len++;
                }
                if (precision == 0 && num == 0) {
                    len = 0;
                }
                len_prec = len;
                len_width = len_prec;
                if (precision >= 0 && (unsigned int) precision > len) {
                    len_prec = precision;
                    len_width = precision;
                }
                if ((flags & 0b00001000) | (flags & 0b00000100)) {
                    len_width++;
                }
                if (flags & 0b00000010) {
                    len_width += 2;
                }
                if ((unsigned int) width < len_width) {
                    width = len_width;
                }
                if (maxrem < (unsigned int) width) {
                    // TODO: Set errno to EOVERFLOW.
                    return -1;
                }
                char filler = ' ';
                if (!((flags & 0b00010000) | (flags & 0b00000001))) {
                    for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                        if (!print(obj, &filler, 1)) {
                            return -1;
                        }
                    }
                }
                if (flags & 0b00001000) {
                    char plus = '+';
                    if (!print(obj, &plus, 1)) {
                        return -1;
                    }
                } else if (flags & 0b00000100) {
                    char space = ' ';
                    if (!print(obj, &space, 1)) {
                        return -1;
                    }
                }
                if (flags & 0b00000010) {
                    char zero = '0';
                    if (!print(obj, &zero, 1)) {
                        return -1;
                    }
                    if (!print(obj, &base_char, 1)) {
                        return -1;
                    }
                }
                if (!(flags & 0b00010000) && (flags & 0b00000001)) {
                    filler = '0';
                    for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                        if (!print(obj, &filler, 1)) {
                            return -1;
                        }
                    }
                }
                while (len < len_prec) {
                    char zero = '0';
                    if (!print(obj, &zero, 1)) {
                        return -1;
                    }
                    len++;
                }
                if (num == 0) {
                    if (precision != 0) {
                        char zero = '0';
                        if (!print(obj, &zero, 1)) {
                            return -1;
                        }
                    }
                } else {
                    char digit;
                    while (div > 15) {
                        uint64_t n = num / div;
                        div /= 16;
                        digit = n % 16;
                        if (digit < 10) {
                            digit += '0';
                        } else {
                            digit += base_char - ('x' - 'a') - 10;
                        }
                        if (!print(obj, &digit, 1)) {
                            return -1;
                        }
                    }
                    digit = num % 16;
                    if (digit < 10) {
                        digit += '0';
                    } else {
                        digit += base_char - ('x' - 'a') - 10;
                    }
                    if (!print(obj, &digit, 1)) {
                        return -1;
                    }
                }
                if (flags & 0b00010000) {
                    for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                        if (!print(obj, &filler, 1)) {
                            return -1;
                        }
                    }
                }
                written += width;
                format++;
            }
#else
            }
#endif
        } else if (*format == 'u') {
            format++;
            unsigned int num = va_arg(args, unsigned int);
            size_t len = 1;
            size_t len_prec;
            size_t len_width;
            unsigned int div = 1;
            while (num / div > 9) {
                div *= 10;
                len++;
            }
            if (precision == 0 && num == 0) {
                len = 0;
            }
            len_prec = len;
            len_width = len_prec;
            if (precision >= 0 && (unsigned int) precision > len) {
                len_prec = precision;
                len_width = precision;
            }
            if ((flags & 0b00001000) | (flags & 0b00000100)) {
                len_width++;
            }
            if (flags & 0b00000010) {
                len_width += 2;
            }
            if ((unsigned int) width < len_width) {
                width = len_width;
            }
            if (maxrem < (unsigned int) width) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            char filler = ' ';
            if (!((flags & 0b00010000) | (flags & 0b00000001))) {
                for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                    if (!print(obj, &filler, 1)) {
                        return -1;
                    }
                }
            }
            if (flags & 0b00001000) {
                char plus = '+';
                if (!print(obj, &plus, 1)) {
                    return -1;
                }
            } else if (flags & 0b00000100) {
                char space = ' ';
                if (!print(obj, &space, 1)) {
                    return -1;
                }
            }
            if (flags & 0b00000010) {
                char zero = '0';
                if (!print(obj, &zero, 1)) {
                    return -1;
                }
                if (!print(obj, format, 1)) {
                    return -1;
                }
            }
            if (!(flags & 0b00010000) && (flags & 0b00000001)) {
                filler = '0';
                for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                    if (!print(obj, &filler, 1)) {
                        return -1;
                    }
                }
            }
            while (len < len_prec) {
                char zero = '0';
                if (!print(obj, &zero, 1)) {
                    return -1;
                }
                len++;
            }
            if (num == 0) {
                if (precision != 0) {
                    char zero = '0';
                    if (!print(obj, &zero, 1)) {
                        return -1;
                    }
                }
            } else {
                char digit;
                while (div > 9) {
                    unsigned int n = num / div;
                    div /= 10;
                    digit = n % 10 + '0';
                    if (!print(obj, &digit, 1)) {
                        return -1;
                    }
                }
                digit = num % 10 + '0';
                if (!print(obj, &digit, 1)) {
                    return -1;
                }
            }
            if (flags & 0b00010000) {
                for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                    if (!print(obj, &filler, 1)) {
                        return -1;
                    }
                }
            }
            written += width;
        } else if (*format == 'd' || *format == 'i') {
            format++;
            int num = va_arg(args, int);
            size_t len = 1;
            size_t len_prec;
            size_t len_width;
            unsigned int div = 1;
            int abs_num = num < 0 ? -num : num;
            while (abs_num / div > 9) {
                div *= 10;
                len++;
            }
            if (precision == 0 && num == 0) {
                len = 0;
            }
            len_prec = len;
            len_width = len_prec;
            if (precision >= 0 && (unsigned int) precision > len) {
                len_prec = precision;
                len_width = precision;
            }
            if ((flags & 0b00001000) | (flags & 0b00000100)) {
                len_width++;
            }
            if (flags & 0b00000010) {
                len_width += 2;
            }
            if (num < 0) {
                len_width++;
            }
            if ((unsigned int) width < len_width) {
                width = len_width;
            }
            if (maxrem < (unsigned int) width) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            char filler = ' ';
            if (!((flags & 0b00010000) | (flags & 0b00000001))) {
                for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                    if (!print(obj, &filler, 1)) {
                        return -1;
                    }
                }
            }
            if (num >= 0) {
                if (flags & 0b00001000) {
                    char plus = '+';
                    if (!print(obj, &plus, 1)) {
                        return -1;
                    }
                } else if (flags & 0b00000100) {
                    char space = ' ';
                    if (!print(obj, &space, 1)) {
                        return -1;
                    }
                }
            } else {
                char minus = '-';
                if (!print(obj, &minus, 1)) {
                    return -1;
                }
                num = -num;
            }
            if (flags & 0b00000010) {
                char zero = '0';
                if (!print(obj, &zero, 1)) {
                    return -1;
                }
                if (!print(obj, format, 1)) {
                    return -1;
                }
            }
            if (!(flags & 0b00010000) && (flags & 0b00000001)) {
                filler = '0';
                for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                    if (!print(obj, &filler, 1)) {
                        return -1;
                    }
                }
            }
            while (len < len_prec) {
                char zero = '0';
                if (!print(obj, &zero, 1)) {
                    return -1;
                }
                len++;
            }
            if (num == 0) {
                if (precision != 0) {
                    char zero = '0';
                    if (!print(obj, &zero, 1)) {
                        return -1;
                    }
                }
            } else {
                char digit;
                while (div > 9) {
                    unsigned int n = abs_num / div;
                    div /= 10;
                    digit = n % 10 + '0';
                    if (!print(obj, &digit, 1)) {
                        return -1;
                    }
                }
                digit = num % 10 + '0';
                if (!print(obj, &digit, 1)) {
                    return -1;
                }
            }
            if (flags & 0b00010000) {
                for (unsigned int i = 0; i < (unsigned int) width - len_width; i++) {
                    if (!print(obj, &filler, 1)) {
                        return -1;
                    }
                }
            }
            written += width;
        }
#ifdef __SSE__
        else if (*format == 'g' || *format == 'f' || *format == 'G' || *format == 'F') {
            format++;
            double num = va_arg(args, double);

            size_t len = 1;
            double div = 1.0;
            while (num / div >= 10.0) {
                div *= 10.0;
                len++;
            }

            size_t total_len = len;
            total_len += di::max(0, precision + 1);

            while (total_len < (size_t) width) {
                char space = ' ';
                if (!print(obj, &space, 1)) {
                    return -1;
                }
                written++;
                width--;
            }

            if (num < 0) {
                num *= -1.0;
                char minus = '-';
                if (!print(obj, &minus, 1)) {
                    return -1;
                }
                written++;
            }

            for (size_t i = 0; i < total_len; i++) {
                if (i == len) {
                    char d = '.';
                    if (!print(obj, &d, 1)) {
                        return -1;
                    }
                    i++;
                }
                double current = num / div;
                int digit = ((int) current) % 10;
                char c = digit + '0';
                if (!print(obj, &c, 1)) {
                    return -1;
                }
                div /= 10;
            }

            written += total_len;
        }
#endif /* __SSE__ */
        else {
            format = format_begun_at;
            size_t len = strlen(format);
            if (maxrem < len) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (!print(obj, format, len)) {
                return -1;
            }
            written += len;
            format += len;
        }
    }

    return written;
}
}
