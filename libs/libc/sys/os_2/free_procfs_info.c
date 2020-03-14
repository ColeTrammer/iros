#include <stdlib.h>
#include <sys/os_2.h>

void free_procfs_info(struct proc_info *info) {
    free(info);
}