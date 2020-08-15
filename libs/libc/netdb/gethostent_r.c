#include <bits/field_parser.h>
#include <netdb.h>
#include <stdio.h>

extern FILE *__hostent_file;

static struct field_descriptor hostent_fields[3] = {
    {
        .type = FIELD_IP_ADDRESS,
        .offset1 = offsetof(struct hostent, h_addr_list),
        .offset2 = offsetof(struct hostent, h_addrtype),
        .offset3 = offsetof(struct hostent, h_length),
    },
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct hostent, h_name),
    },
    {
        .type = FIELD_STRING_ARRAY,
        .offset1 = offsetof(struct hostent, h_aliases),
        .arg1_s = " \t",                                          /* Space separated */
        .flags = FIELD_DEFAULT_IF_NOT_PRESENT | FIELD_DONT_SPLIT, /* Empty if not present, don't split on top level separator */
    },
};

static struct field_parser_info hostent_field_info = {
    .field_count = sizeof(hostent_fields) / sizeof(hostent_fields[0]),
    .separator = " \t",
    .fields = hostent_fields,
};

int gethostent_r(struct hostent *hostent, char *buf, size_t buflen, struct hostent **result, int *h_errnop) {
    if (!__hostent_file) {
        sethostent(0);
        if (!__hostent_file) {
            *h_errnop = NO_DATA;
            *result = NULL;
            return -1;
        }
    }

    int ret = __parse_fields(&hostent_field_info, hostent, buf, buflen, __hostent_file);
    if (ret < hostent_field_info.field_count) {
        *h_errnop = NO_DATA;
        *result = NULL;
        return -1;
    }

    *result = hostent;
    return 0;
}
