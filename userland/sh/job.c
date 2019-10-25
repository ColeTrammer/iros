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
    printf("[%d]+ %5d %s\n", jid, job.pgid,
        job.state == RUNNING ? "Running" : 
        job.state == STOPPED ? "Stopped" :
        "Terminated");
}

void job_print_all() {
    for (pid_t i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].state != DNE) {
            print_job(i + 1);
        }
    }
}

void job_add(pid_t pgid, int num_prcessed, enum job_state state) {
    for (size_t i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].state == DNE) {
            jobs[i].pgid = pgid;
            jobs[i].state = state;
            jobs[i].num_processes = num_prcessed;
            jobs[i].num_consumed = 1;
            return;
        }
    }
}

struct job_id job_id(enum job_id_type type, pid_t id) {
    return (struct job_id) { type, { id } };
}

// Put into fg
int job_run(struct job_id id) {
    pid_t jid;
    if (id.type == JOB_ID) {
        jid = id.id.jid;
    } else {
        jid = get_jid_from_pgid(id.id.pgid);
    }

    if (jid <= 0 || jid > MAX_JOBS) {
        return 1;
    }

    struct job *job = &jobs[jid - 1];
    assert(job->state == STOPPED);

    pid_t save_pgid = getpid();
    if (isatty(STDOUT_FILENO)) {
        tcsetpgrp(STDOUT_FILENO, job->pgid);
    }

    if (killpg(job->pgid, SIGCONT)) {
        return 1;
    }

    int status;
    while (job->num_processes > 0) {
        // The wait signal wasn't just a by product of killpg SIGSTOP
        if (job->num_consumed > job->num_processes) {
            job->num_consumed = 1;
            killpg(job->pgid, SIGSTOP);
            printf("%c", '\n');
            print_job(jid);
            break;
        }

        pid_t ret;
        while (!(ret = waitpid(-job->pgid, &status, WUNTRACED)));

        if (ret == -1) {
            assert(false);
        }

        if (WIFEXITED(status)) {
            job->num_processes--;
            job->num_consumed--;
        } else if (WIFSTOPPED(status)) {
            job->num_consumed++;
        } else {
            assert(false);
        }
    }

    if (isatty(STDOUT_FILENO)) {
        tcsetpgrp(STDOUT_FILENO, save_pgid);
    }

    if (job->num_processes <= 0) {
        job->state = TERMINATED;
        print_job(jid);
        job->state = DNE;
    }

    return WEXITSTATUS(status);
}