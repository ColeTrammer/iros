#include <arpa/inet.h>
#include <bits/field_parser.h>
#include <netdb.h>
#include <stdio.h>

extern FILE *__serv_file;

static struct field_descriptor servent_fields[4] = {
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct servent, s_name),
    },
    {
        .type = FIELD_NUMBER,
        .offset1 = offsetof(struct servent, s_port),
        .arg1_l = sizeof(int),
        .separator_override = "/", /* Port and protocol are separated by a '/' */
    },
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct servent, s_proto),
    },
    {
        .type = FIELD_STRING_ARRAY,
        .offset1 = offsetof(struct servent, s_aliases),
        .arg1_s = " \t",                                          /* Space separated */
        .flags = FIELD_DEFAULT_IF_NOT_PRESENT | FIELD_DONT_SPLIT, /* Empty if not present, don't split on top level separator */
    },
};

static struct field_parser_info servent_field_info = {
    .field_count = sizeof(servent_fields) / sizeof(servent_fields[0]),
    .separator = " \t",
    .fields = servent_fields,
};

int getservent_r(struct servent *servent, char *buf, size_t buflen, struct servent **result) {
    if (!__serv_file) {
        setservent(0);
        if (!__serv_file) {
            *result = NULL;
            return -1;
        }
    }

    int ret = __parse_fields(&servent_field_info, servent, buf, buflen, __serv_file);
    if (ret < servent_field_info.field_count) {
        *result = NULL;
        return -1;
    }

    // Return the port in netork byte order.
    servent->s_port = htonl(servent->s_port);
    *result = servent;
    return 0;
}
