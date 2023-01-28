#include <bits/field_parser.h>
#include <pwd.h>
#include <stdio.h>

FILE *__pwd_file = NULL;

static struct field_descriptor pwd_fields[7] = {
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct passwd, pw_name),
        .flags = FIELD_ALLOW_EMPTY,
    },
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct passwd, pw_passwd),
        .flags = FIELD_ALLOW_EMPTY,
    },
    {
        .type = FIELD_NUMBER,
        .offset1 = offsetof(struct passwd, pw_uid),
        .arg1_l = sizeof(uid_t),
        .flags = FIELD_ALLOW_EMPTY,
    },
    {
        .type = FIELD_NUMBER,
        .offset1 = offsetof(struct passwd, pw_gid),
        .arg1_l = sizeof(gid_t),
        .flags = FIELD_ALLOW_EMPTY,
    },
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct passwd, pw_gecos),
        .flags = FIELD_ALLOW_EMPTY,
    },
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct passwd, pw_dir),
        .flags = FIELD_ALLOW_EMPTY,
    },
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct passwd, pw_shell),
        .flags = FIELD_ALLOW_EMPTY,
    },
};

static struct field_parser_info pwd_field_info = {
    .field_count = sizeof(pwd_fields) / sizeof(pwd_fields[0]),
    .separator = ":",
    .fields = pwd_fields,
    .flags = FIELD_PARSER_NO_COMMENTS,
};

int getpwent_r(struct passwd *pwbuf, char *buf, size_t buflen, struct passwd **result) {
    if (!__pwd_file) {
        __pwd_file = fopen("/etc/passwd", "r");
        if (!__pwd_file) {
            *result = NULL;
            return -1;
        }
    }

    int ret = __parse_fields(&pwd_field_info, pwbuf, buf, buflen, __pwd_file);
    if (ret < pwd_field_info.field_count) {
        *result = NULL;
        return -1;
    }

    *result = pwbuf;
    return 0;
}
