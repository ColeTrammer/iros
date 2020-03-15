#include <stdio.h>
#include <strings.h>
#include <sys/os_2.h>

int read_procfs_global_info(struct proc_global_info *info) {
    FILE *file = fopen("/proc/sched", "r");
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

    return fclose(file);

fail:
    fclose(file);
    return 1;
}
