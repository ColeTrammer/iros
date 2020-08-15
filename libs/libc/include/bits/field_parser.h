#ifndef _BITS_FIELD_PARSER_H
#define _BITS_FIELD_PARSER_H 1

#include <stdio.h>

/*
    STRING: no extra info
    STRING_ARRAY: arg1 = separator character for array
    IP_ADDRESS: offset2 = type (ipv4 or ipv6), offset3 = address length
    FIELD_NUMBER: no extra info
*/

enum field_type {
    FIELD_STRING,
    FIELD_STRING_ARRAY,
    FIELD_IP_ADDRESS,
};

struct field_descriptor {
    enum field_type type;
#define FIELD_DEFAULT_IF_NOT_PRESENT 1
    int flags;
    int arg1;
    int offset1;
    int offset2;
    int offset3;
};

struct field_parser_info {
    struct field_descriptor *fields;
    int field_count;
    char separator;
};

// Returns the number of fields read (like scanf), and -1 on read errors.
int __parse_fields(struct field_parser_info *info, void *object, void *buffer, size_t buffer_max, FILE *file);

#endif /* _BITS_FIELD_PARSER_H */
