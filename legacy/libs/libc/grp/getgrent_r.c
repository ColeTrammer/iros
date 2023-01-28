#include <bits/field_parser.h>
#include <grp.h>
#include <stdio.h>

FILE *__group_file = NULL;

static struct field_descriptor grp_fields[4] = {
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct group, gr_name),
        .flags = FIELD_ALLOW_EMPTY,
    },
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct group, gr_passwd),
        .flags = FIELD_ALLOW_EMPTY,
    },
    {
        .type = FIELD_NUMBER,
        .offset1 = offsetof(struct group, gr_gid),
        .arg1_l = sizeof(gid_t),
        .flags = FIELD_ALLOW_EMPTY,
    },
    {
        .type = FIELD_STRING_ARRAY,
        .offset1 = offsetof(struct group, gr_mem),
        .arg1_s = ",",
        .flags = FIELD_ALLOW_EMPTY,
    },
};

static struct field_parser_info grp_field_info = {
    .field_count = sizeof(grp_fields) / sizeof(grp_fields[0]),
    .separator = ":",
    .fields = grp_fields,
    .flags = FIELD_PARSER_NO_COMMENTS,
};

int getgrent_r(struct group *group, char *buf, size_t buflen, struct group **result) {
    if (!__group_file) {
        __group_file = fopen("/etc/group", "r");
        if (!__group_file) {
            *result = NULL;
            return -1;
        }
    }

    int ret = __parse_fields(&grp_field_info, group, buf, buflen, __group_file);
    if (ret < grp_field_info.field_count) {
        *result = NULL;
        return -1;
    }

    *result = group;
    return 0;
}
