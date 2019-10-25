#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#include "command.h"
#include "job.h"

static struct job jobs[MAX_JOBS];

pid_t get_pgid_from_jid(pid_t jid) {
    struct job job = jobs[jid - 1];
    return job.state == DNE ? -1 : job.pgid;
}

pid_t get_jid_from_pgid(pid_t pgid) {
    for (pid_t i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pgid != DNE && jobs[i].pgid == pgid) {
            return i + 1;
        }
    }

    return -1;
}

void print_job(pid_t jid) {
    assert(jid > 0 && jid < MAX_JOBS);
    struct job job  = jobs[jid - 1];
    assert(job.state != DNE);
    printf("[%d] %5d %s\n", jid, job.pgid,
        job.state == RUNNING ? "Running" : 
        job.state == STOPPED ? "Stopped" :
        "Terminated");
}

void job_print_all() {
    for (pid_t i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pgid != DNE) {
            print_job(i + 1);
        }
    }
}

void job_add(pid_t pgid, enum job_state state) {
    for (size_t i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].state == DNE) {
            jobs[i].pgid = pgid;
            jobs[i].state = state;
            return;
        }
    }
}

struct job_id job_id(enum job_id_type type, pid_t id) {
    return (struct job_id) { type, { id } };
}

// Put into fg
int job_run(struct job_id id) {
    pid_t pgid = id.id.pgid;
    if (id.type == JOB_ID) {
        pgid = get_pgid_from_jid(id.id.jid);
    }

    pid_t save_pgid = getpid();
    if (isatty(STDOUT_FILENO)) {
        tcsetpgrp(STDOUT_FILENO, pgid);
    }

    if (pgid < 0 || killpg(pgid, SIGCONT)) {
        return 1;
    }

    pid_t ret;
    int status;
    while (!(ret = waitpid(-pgid, &status, WUNTRACED)));

    if (WIFSTOPPED(status)) {
        assert(false);
    }

    if (isatty(STDOUT_FILENO)) {
        tcsetpgrp(STDOUT_FILENO, save_pgid);
    }

    return WEXITSTATUS(status);
}