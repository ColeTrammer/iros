#include "../builtin.h"
#include "../job.h"

static int op_fg(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <job>\n", argv[0]);
        return 0;
    }

    job_check_updates(true);

    struct job_id id;
    if (argv[1][0] == '%') {
        id = job_id(JOB_ID, atoi(argv[1] + 1));
    } else {
        id = job_id(JOB_PGID, atoi(argv[1]));
    }

    return job_run(id);
}
SH_REGISTER_BUILTIN(fg, op_fg);
