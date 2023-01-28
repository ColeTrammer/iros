#include <bits/field_parser.h>
#include <mntent.h>

static struct field_descriptor mntent_fields[4] = {
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct mntent, mnt_fsname),
    },
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct mntent, mnt_dir),
    },
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct mntent, mnt_type),
    },
    {
        .type = FIELD_STRING,
        .offset1 = offsetof(struct mntent, mnt_opts),
        .flags = FIELD_DEFAULT_IF_NOT_PRESENT,
    },
};

static struct field_parser_info mntent_field_info = {
    .field_count = sizeof(mntent_fields) / sizeof(mntent_fields[0]),
    .separator = " \t",
    .fields = mntent_fields,
};

static struct mntent static_mntent;
static char static_mntent_buffer[2048];

struct mntent *getmntent(FILE *file) {
    int ret = __parse_fields(&mntent_field_info, &static_mntent, static_mntent_buffer, sizeof(static_mntent_buffer), file);
    if (ret < mntent_field_info.field_count) {
        return NULL;
    }

    return &static_mntent;
}
