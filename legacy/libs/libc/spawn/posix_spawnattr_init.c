#include <spawn.h>

int posix_spawnattr_init(posix_spawnattr_t *attr) {
    *attr = (posix_spawnattr_t) {
        .__sched_param = {
            .sched_priority = 0,
        },
        .__signal_mask = 0,
        .__signal_default = 0,
        .__sched_policy = SCHED_OTHER,
        .__process_group = 0,
        .__flags = 0,
    };
    return 0;
}
