#include <procinfo.h>
#include <stdlib.h>

void free_procfs_info(struct proc_info *info) {
#ifndef __iros__
    (void) info;
#else
    free(info);
#endif /* __iros__ */
}
