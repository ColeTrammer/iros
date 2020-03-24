#include <procinfo.h>
#include <stdio.h>
#include <strings.h>

int read_procfs_global_info(struct proc_global_info *info, int flags) {
    FILE *file = NULL;
    if (flags & READ_PROCFS_GLOBAL_SCHED) {
        file = fopen("/proc/sched", "r");
        if (!file) {
            return 1;
        }

#define READ_ENTRY(name, spec)                                             \
    do {                                                                   \
        char id[64];                                                       \
        if (fscanf(file, "%64[^:]: " spec, id, &info->name) != 2) {        \
            fprintf(stderr, "Failed to read " #name);                      \
            goto fail;                                                     \
        }                                                                  \
        if (strcasecmp(id, "" #name) != 0) {                               \
            fprintf(stderr, "Expected: " #name " but got %s instead", id); \
            goto fail;                                                     \
        }                                                                  \
    } while (0)

        READ_ENTRY(idle_ticks, "%lu");
        READ_ENTRY(user_ticks, "%lu");
        READ_ENTRY(kernel_ticks, "%lu");

        if (fclose(file)) {
            return 1;
        }
    }

    if (flags & READ_PROCFS_GLOBAL_MEMINFO) {
        file = fopen("/proc/meminfo", "r");
        if (!file) {
            return 1;
        }

        READ_ENTRY(total_memory, "%lu");

        if (fclose(file)) {
            return 1;
        }
    }

    return 0;

fail:
    fclose(file);
    return 1;
}
