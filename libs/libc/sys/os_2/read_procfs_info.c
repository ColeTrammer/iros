#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/os_2.h>

static int scandir_filter(const struct dirent *ent) {
    for (size_t i = 0; ent->d_name[i] != '\0'; i++) {
        if (!isdigit(ent->d_name[i])) {
            return 0;
        }
    }

    return 1;
}

static int sort_by_pid(const struct dirent **a, const struct dirent **b) {
    int a_pid = atoi((*a)->d_name);
    int b_pid = atoi((*b)->d_name);
    if (a_pid < b_pid) {
        return -1;
    } else if (a_pid == b_pid) {
        return 0;
    } else {
        return 1;
    }
}

int read_procfs_info(struct proc_info **info, size_t *length, int flags __attribute__((unused))) {
    assert(info);
    assert(length);

    struct dirent **pids;
    int num_pids;
    if ((num_pids = scandir("/proc", &pids, scandir_filter, sort_by_pid)) == -1) {
        return 1;
    }

    *length = num_pids;
    *info = malloc(num_pids * sizeof(struct proc_info));
    assert(*info);

    bool success = true;
    for (int i = 0; i < num_pids; i++) {
        struct dirent *dirent = pids[i];
        if (!success) {
            free(dirent);
            continue;
        }

        char path_buf[1024];
        snprintf(path_buf, sizeof(path_buf) - 1, "/proc/%s/status", dirent->d_name);

        FILE *status = fopen(path_buf, "r");
        if (!status) {
            goto read_procfs_info_fail;
        }

#define _(x) "" #x
#define READ_PROCFS_STRING_FIELD(name, convert_spec)                                                                 \
    do {                                                                                                             \
        char field_name[64];                                                                                         \
        if (fscanf(status, "%64[^:]: " convert_spec, field_name, (*info)[i].name) != 2) {                            \
            fprintf(stderr, "fscanf: failed to read field `%s`\n", _(name));                                         \
            goto read_procfs_info_fail;                                                                              \
        }                                                                                                            \
        if (strcasecmp(field_name, _(name)) != 0) {                                                                  \
            fprintf(stderr, "Error: expected `%s` but got `%s` when reading `%s`\n", _(name), field_name, path_buf); \
            goto read_procfs_info_fail;                                                                              \
        }                                                                                                            \
    } while (0)
#define READ_PROCFS_FIELD(name, convert_spec)                                                                        \
    do {                                                                                                             \
        char field_name[64];                                                                                         \
        if (fscanf(status, "%64[^:]: " convert_spec, field_name, &(*info)[i].name) != 2) {                           \
            fprintf(stderr, "fscanf: failed to read field `%s`\n", _(name));                                         \
            goto read_procfs_info_fail;                                                                              \
        }                                                                                                            \
        if (strcasecmp(field_name, _(name)) != 0) {                                                                  \
            fprintf(stderr, "Error: expected `%s` but got `%s` when reading `%s`\n", _(name), field_name, path_buf); \
            goto read_procfs_info_fail;                                                                              \
        }                                                                                                            \
    } while (0)

        READ_PROCFS_STRING_FIELD(name, "%64s");
        READ_PROCFS_FIELD(pid, "%d");
        READ_PROCFS_STRING_FIELD(state, "%64[^\n]");
        READ_PROCFS_FIELD(uid, "%hu");
        READ_PROCFS_FIELD(gid, "%hu");
        READ_PROCFS_FIELD(ppid, "%d");
        READ_PROCFS_FIELD(umask, "%o");
        READ_PROCFS_FIELD(euid, "%hu");
        READ_PROCFS_FIELD(egid, "%hu");
        READ_PROCFS_FIELD(pgid, "%d");
        READ_PROCFS_FIELD(sid, "%d");
        READ_PROCFS_STRING_FIELD(tty, "%64s");

        if (fclose(status)) {
            goto read_procfs_info_fail;
        }

        free(dirent);
        continue;

    read_procfs_info_fail:
        free(dirent);
        success = false;
        continue;
    }

    free(pids);
    return success ? 0 : 1;
}