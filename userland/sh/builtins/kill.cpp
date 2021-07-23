#include <signal.h>
#include <stdio.h>

#include "../builtin.h"
#include "../job.h"

static int op_kill(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <job>\n", argv[0]);
        return 0;
    }

    struct job_id id;
    if (argv[1][0] == '%') {
        id = job_id(JOB_ID, atoi(argv[1] + 1));
    } else {
        id = job_id(JOB_PGID, atoi(argv[1]));
    }

    int ret = killpg(get_pgid_from_id(id), SIGTERM);

    job_check_updates(true);
    return ret;
}
SH_REGISTER_BUILTIN(kill, op_kill);
