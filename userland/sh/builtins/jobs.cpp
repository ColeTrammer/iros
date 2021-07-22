#include "../builtin.h"
#include "../job.h"

static int op_jobs(char **argv) {
    (void) argv;

    job_check_updates(false);
    job_print_all();
    return 0;
}
SH_REGISTER_BUILTIN(jobs, op_jobs);
