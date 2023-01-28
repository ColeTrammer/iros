#include <bits/field_parser.h>
#include <netdb.h>
#include <stdio.h>

extern FILE *__proto_file;

static struct field_descriptor protoent_fields[3] = {
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct protoent, p_name),
    },
    {
        .type = FIELD_NUMBER,
        .offset1 = offsetof(struct protoent, p_proto),
        .arg1_l = sizeof(int),
    },
    {
        .type = FIELD_STRING_ARRAY,
        .offset1 = offsetof(struct protoent, p_aliases),
        .arg1_s = " \t",                                          /* Space separated */
        .flags = FIELD_DEFAULT_IF_NOT_PRESENT | FIELD_DONT_SPLIT, /* Empty if not present, don't split on top level separator */
    },
};

static struct field_parser_info protoent_field_info = {
    .field_count = sizeof(protoent_fields) / sizeof(protoent_fields[0]),
    .separator = " \t",
    .fields = protoent_fields,
};

int getprotoent_r(struct protoent *protoent, char *buf, size_t buflen, struct protoent **result) {
    if (!__proto_file) {
        setprotoent(0);
        if (!__proto_file) {
            *result = NULL;
            return -1;
        }
    }

    int ret = __parse_fields(&protoent_field_info, protoent, buf, buflen, __proto_file);
    if (ret < protoent_field_info.field_count) {
        *result = NULL;
        return -1;
    }

    *result = protoent;
    return 0;
}
