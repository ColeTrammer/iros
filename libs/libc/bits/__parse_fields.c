#include <arpa/inet.h>
#include <assert.h>
#include <bits/field_parser.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define TRY_WRITE_BUF(data, buffer, i, max)              \
    ({                                                   \
        if ((i) + sizeof(data) > (max)) {                \
            return -1;                                   \
        }                                                \
        *(__typeof__(data) *) ((buffer) + (i)) = (data); \
        (i) += sizeof(data);                             \
        (buffer) + (i - sizeof(data));                   \
    })

#define TRY_WRITE_STRING(s, buffer, i, max) \
    ({                                      \
        size_t __slen = strlen(s) + 1;      \
        if ((i) + __slen > (max)) {         \
            return -1;                      \
        }                                   \
        memcpy((buffer) + (i), s, __slen);  \
        (i) += __slen;                      \
        (buffer) + ((i) -__slen);           \
    })

#define WRITE_STRUCTURE(object, offset, value)                             \
    do {                                                                   \
        *(__typeof__(value) *) (((void *) (object)) + (offset)) = (value); \
    } while (0)

#define WRITE_STRUCTURE_NUMBER(object, offset, value, number_size) \
    do {                                                           \
        switch (number_size) {                                     \
            case sizeof(uint8_t): {                                \
                uint8_t number = (uint8_t) value;                  \
                WRITE_STRUCTURE(object, offset, number);           \
                break;                                             \
            }                                                      \
            case sizeof(uint16_t): {                               \
                uint16_t number = (uint16_t) value;                \
                WRITE_STRUCTURE(object, offset, number);           \
                break;                                             \
            }                                                      \
            case sizeof(uint32_t): {                               \
                uint32_t number = (uint32_t) value;                \
                WRITE_STRUCTURE(object, offset, number);           \
                break;                                             \
            }                                                      \
            case sizeof(uint64_t): {                               \
                uint64_t number = (uint64_t) value;                \
                WRITE_STRUCTURE(object, offset, number);           \
                break;                                             \
            }                                                      \
            default:                                               \
                assert(false);                                     \
        }                                                          \
    } while (0)

int __parse_fields(struct field_parser_info *info, void *object, void *buffer, size_t buffer_max, FILE *file) {
    bool allow_comments = !(info->flags & FIELD_PARSER_NO_COMMENTS);
    for (;;) {
        char line_buffer[_POSIX2_LINE_MAX];
        char *line = fgets(line_buffer, sizeof(line_buffer), file);
        if (!line) {
            return -1;
        }

        size_t buffer_index = 0;
        size_t line_index = 0;
        int field_index = 0;
        char *field = NULL;
        for (; field_index < info->field_count;) {
            struct field_descriptor *field_desc = &info->fields[field_index];

            bool done_with_line = line[line_index] == '\n' || line[line_index] == '\0' || (line[line_index] == '#' && allow_comments);
            bool is_separator = !!strchr(info->separator, line[line_index]) && !(field_desc->flags & FIELD_DONT_SPLIT);
            if (!is_separator && !done_with_line) {
                if (!field) {
                    field = &line[line_index];
                }
                line_index++;
                continue;
            }

            if (!field && !!(field_desc->flags & FIELD_ALLOW_EMPTY)) {
                field = "";
            }

            if (!field) {
                if (done_with_line) {
                    break;
                }
                line_index++;
                continue;
            }

            line[line_index++] = '\0';

            switch (field_desc->type) {
                case FIELD_STRING: {
                    char *string = TRY_WRITE_STRING(field, buffer, buffer_index, buffer_max);
                    WRITE_STRUCTURE(object, field_desc->offset1, string);
                    break;
                }
                case FIELD_STRING_ARRAY: {
                    char *separator = field_desc->arg1_s;

                    // Count the number of array elements.
                    size_t count = 0;
                    bool started_field = false;
                    for (size_t i = 0;; i++) {
                        bool is_terminator = field[i] == '\0' || (field[i] == '#' && allow_comments);
                        bool is_separator = !!strchr(separator, field[i]) || is_terminator;
                        if (is_separator && started_field) {
                            started_field = false;
                            count++;
                        } else if (!is_separator && !started_field) {
                            started_field = true;
                        }

                        if (is_terminator) {
                            break;
                        }
                    }

                    // Reserve space for the array (including terminating NULL)
                    char **array = TRY_WRITE_BUF(NULL, buffer, buffer_index, buffer_max);
                    for (size_t i = 0; i < count; i++) {
                        TRY_WRITE_BUF(NULL, buffer, buffer_index, buffer_max);
                    }

                    char *element = NULL;
                    size_t array_index = 0;
                    for (size_t i = 0;; i++) {
                        bool is_terminator = field[i] == '\0' || (field[i] == '#' && allow_comments);
                        bool is_separator = !!strchr(separator, field[i]) || is_terminator;
                        if (!is_separator && !element) {
                            element = &field[i];
                            continue;
                        } else if (is_separator && !!element) {
                            field[i] = '\0';
                            array[array_index++] = TRY_WRITE_STRING(element, buffer, buffer_index, buffer_max);
                            element = NULL;
                        }

                        if (is_terminator) {
                            break;
                        }
                    }
                    WRITE_STRUCTURE(object, field_desc->offset1, array);
                    break;
                }
                case FIELD_IP_ADDRESS: {
                    struct in_addr addr;
                    if (inet_aton(field, &addr) == 0) {
                        return -1;
                    }
                    int type = AF_INET;
                    int addrlen = sizeof(in_addr_t);

                    char *addr_p = TRY_WRITE_BUF(addr.s_addr, buffer, buffer_index, buffer_max);
                    char **addresses = TRY_WRITE_BUF(addr_p, buffer, buffer_index, buffer_max);
                    TRY_WRITE_BUF(NULL, buffer, buffer_index, buffer_max);

                    WRITE_STRUCTURE(object, field_desc->offset1, addresses);
                    WRITE_STRUCTURE(object, field_desc->offset2, type);
                    WRITE_STRUCTURE(object, field_desc->offset3, addrlen);
                    break;
                }
                case FIELD_NUMBER: {
                    char *end;
                    long value = strtol(field, &end, 10);
                    if (!end || !!*end) {
                        return -1;
                    }
                    WRITE_STRUCTURE_NUMBER(object, field_desc->offset1, value, field_desc->arg1_l);
                    break;
                }
                default:
                    assert(false);
                    break;
            }

            field_index++;
            field = NULL;

            if (done_with_line) {
                break;
            }
        }

        // No fields were read, try the next line.
        if (field_index == 0) {
            continue;
        }

        // Give fields a default value if requested.
        for (; field_index < info->field_count; field_index++) {
            struct field_descriptor *field_desc = &info->fields[field_index];
            if (!(field_desc->flags & FIELD_DEFAULT_IF_NOT_PRESENT)) {
                break;
            }

            switch (field_desc->type) {
                case FIELD_STRING: {
                    char *string = TRY_WRITE_STRING("", buffer, buffer_index, buffer_max);
                    WRITE_STRUCTURE(object, field_desc->offset1, string);
                    break;
                }
                case FIELD_STRING_ARRAY: {
                    char **empty_array = TRY_WRITE_BUF(NULL, buffer, buffer_index, buffer_max);
                    WRITE_STRUCTURE(object, field_desc->offset1, empty_array);
                    break;
                }
                case FIELD_IP_ADDRESS: {
                    char **empty_array = TRY_WRITE_BUF(NULL, buffer, buffer_index, buffer_max);
                    int type = 0;
                    int len = 0;
                    WRITE_STRUCTURE(object, field_desc->offset1, empty_array);
                    WRITE_STRUCTURE(object, field_desc->offset2, type);
                    WRITE_STRUCTURE(object, field_desc->offset3, len);
                    break;
                }
                case FIELD_NUMBER: {
                    WRITE_STRUCTURE_NUMBER(object, field_desc->offset1, 0, field_desc->arg1_l);
                    break;
                }
                default:
                    assert(false);
                    break;
            }
        }

        return field_index;
    }
}
