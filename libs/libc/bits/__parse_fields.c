#include <arpa/inet.h>
#include <assert.h>
#include <bits/field_parser.h>
#include <limits.h>
#include <stdbool.h>
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

int __parse_fields(struct field_parser_info *info, void *object, void *buffer, size_t buffer_max, FILE *file) {
    for (;;) {
        char line_buffer[_POSIX2_LINE_MAX];
        char *line = fgets(line_buffer, sizeof(line_buffer), file);
        if (!line) {
            return -1;
        }

        // Ignore empty lines.
        if (!*line) {
            continue;
        }

        size_t buffer_index = 0;
        size_t line_index = 0;
        int field_index = 0;
        char *field = line;
        for (;;) {
            // Ignore comments
            if (line[line_index] == '#') {
                break;
            } else if (line[line_index] != info->separator && line[line_index] != '\n' && line[line_index] != '\0') {
                line_index++;
                continue;
            }

            char *next_field = (line[line_index] == '\0' || line[line_index] == '\n') ? NULL : &line[line_index + 1];
            line[line_index++] = '\0';

            struct field_descriptor *field_desc = &info->fields[field_index];
            switch (field_desc->type) {
                case FIELD_STRING: {
                    char *string = TRY_WRITE_STRING(field, buffer, buffer_index, buffer_max);
                    WRITE_STRUCTURE(object, field_desc->offset1, string);
                    break;
                }
                case FIELD_STRING_ARRAY: {
                    break;
                }
                case FIELD_IP_ADDRESS: {
                    in_addr_t addr = inet_addr(field);
                    int type = AF_INET;
                    int addrlen = sizeof(in_addr_t);
                    if (addr == INADDR_NONE) {
                        return -1;
                    }

                    char *addr_p = TRY_WRITE_BUF(addr, buffer, buffer_index, buffer_max);
                    char **addresses = TRY_WRITE_BUF(addr_p, buffer, buffer_index, buffer_max);
                    TRY_WRITE_BUF(NULL, buffer, buffer_index, buffer_max);

                    WRITE_STRUCTURE(object, field_desc->offset1, addresses);
                    WRITE_STRUCTURE(object, field_desc->offset2, type);
                    WRITE_STRUCTURE(object, field_desc->offset3, addrlen);
                    break;
                }
                default:
                    assert(false);
                    break;
            }

            while (next_field && *next_field && *next_field == info->separator) {
                next_field++;
                line_index++;
            }

            field_index++;
            if (!next_field || !*next_field || field_index >= info->field_count) {
                break;
            }

            field = next_field;
        }

        // No fields were read, try the next line.
        if (field_index == 0) {
            continue;
        }

        // Give fields a default value if requested.
        for (; field_index < info->field_count; field_index++) {
            struct field_descriptor *field_desc = &info->fields[field_index];
            if (!field_desc->flags & FIELD_DEFAULT_IF_NOT_PRESENT) {
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
                default:
                    assert(false);
                    break;
            }
        }

        return field_index;
    }
}
