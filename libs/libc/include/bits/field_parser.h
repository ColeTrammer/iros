#ifndef _BITS_FIELD_PARSER_H
#define _BITS_FIELD_PARSER_H 1

#include <stdio.h>

/*
    STRING: no extra info
    STRING_ARRAY: arg1 = separator string for array
    IP_ADDRESS: offset2 = type (ipv4 or ipv6), offset3 = address length
*/

enum field_type {
    FIELD_STRING,
    FIELD_STRING_ARRAY,
    FIELD_IP_ADDRESS,
};

struct field_descriptor {
    enum field_type type;
#define FIELD_DEFAULT_IF_NOT_PRESENT 1
#define FIELD_DONT_SPLIT             2
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
    char *separator;
};

// Returns the number of fields read (like scanf), and -1 on read errors.
int __parse_fields(struct field_parser_info *info, void *object, void *buffer, size_t buffer_max, FILE *file);

#endif /* _BITS_FIELD_PARSER_H */
