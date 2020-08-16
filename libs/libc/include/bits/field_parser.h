#ifndef _BITS_FIELD_PARSER_H
#define _BITS_FIELD_PARSER_H 1

#include <stdio.h>

/*
    STRING: no extra info
    STRING_ARRAY: arg1 = separator string for array
    IP_ADDRESS: offset2 = type (ipv4 or ipv6), offset3 = address length
    NUMBER: arg1 = sizeof(number)
    IP_V4_NETWORK: offset1 = ip address, offset2 = address type
*/

enum field_type {
    FIELD_STRING,
    FIELD_STRING_ARRAY,
    FIELD_IP_ADDRESS,
    FIELD_NUMBER,
    FIELD_IP_V4_NETWORK,
};

struct field_descriptor {
    enum field_type type;
#define FIELD_DEFAULT_IF_NOT_PRESENT 1
#define FIELD_DONT_SPLIT             2
#define FIELD_ALLOW_EMPTY            4
    int flags;
    int offset1;
    int offset2;
    int offset3;
    union {
        long arg1_l;
        char *arg1_s;
    };
};

struct field_parser_info {
    struct field_descriptor *fields;
    int field_count;
#define FIELD_PARSER_NO_COMMENTS 1
    int flags;
    char *separator;
};

// Returns the number of fields read (like scanf), and -1 on read errors.
int __parse_fields(struct field_parser_info *info, void *object, void *buffer, size_t buffer_max, FILE *file);

#define TRY_WRITE_BUF(data, buffer, i, max)              \
    ({                                                   \
        if ((i) + sizeof(data) > (max)) {                \
            return -1;                                   \
        }                                                \
        *(__typeof__(data) *) ((buffer) + (i)) = (data); \
        (i) += sizeof(data);                             \
        (void *) (buffer) + (i - sizeof(data));          \
    })

#define TRY_WRITE_STRING(s, buffer, i, max) \
    ({                                      \
        size_t __slen = strlen(s) + 1;      \
        if ((i) + __slen > (max)) {         \
            return -1;                      \
        }                                   \
        memcpy((buffer) + (i), s, __slen);  \
        (i) += __slen;                      \
        (char *) (buffer) + ((i) -__slen);  \
    })

#endif /* _BITS_FIELD_PARSER_H */
