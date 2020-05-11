#include <sys/os_2.h>
#include <sys/syscall.h>

int get_initial_process_info(struct initial_process_info *info) {
    return syscall(SC_GET_INITIAL_PROCESS_INFO, info);
}
