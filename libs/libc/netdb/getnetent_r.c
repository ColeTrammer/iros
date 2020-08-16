#include <bits/field_parser.h>
#include <netdb.h>
#include <stdio.h>

extern FILE *__net_file;

static struct field_descriptor netent_fields[3] = {
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct netent, n_name),
    },
    {
        .type = FIELD_IP_V4_NETWORK,
        .offset1 = offsetof(struct netent, n_net),
        .offset2 = offsetof(struct netent, n_addrtype),
    },
    {
        .type = FIELD_STRING_ARRAY,
        .offset1 = offsetof(struct netent, n_aliases),
        .arg1_s = " \t",                                          /* Space separated */
        .flags = FIELD_DEFAULT_IF_NOT_PRESENT | FIELD_DONT_SPLIT, /* Empty if not present, don't split on top level separator */
    },
};

static struct field_parser_info netent_field_info = {
    .field_count = sizeof(netent_fields) / sizeof(netent_fields[0]),
    .separator = " \t",
    .fields = netent_fields,
};

int getnetent_r(struct netent *netent, char *buf, size_t buflen, struct netent **result) {
    if (!__net_file) {
        setnetent(0);
        if (!__net_file) {
            *result = NULL;
            return -1;
        }
    }

    int ret = __parse_fields(&netent_field_info, netent, buf, buflen, __net_file);
    if (ret < netent_field_info.field_count) {
        *result = NULL;
        return -1;
    }

    *result = netent;
    return 0;
}
