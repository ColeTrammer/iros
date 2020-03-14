#include <assert.h>
#include <ctype.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/os_2.h>
#include <unistd.h>

static bool print_all;
static bool print_extra;

static pid_t our_pid;
static char *tty_name;

void print_usage_and_exit(char **argv) {
    fprintf(stderr, "Usage: %s [-ef]\n", argv[0]);
    exit(2);
}

int main(int argc, char **argv) {

    opterr = 0;
    char opt;
    while ((opt = getopt(argc, argv, "ef")) != -1) {
        switch (opt) {
            case 'e':
                print_all = true;
                break;
            case 'f':
                print_extra = true;
                break;
            case ':':
            case '?':
            default:
                print_usage_and_exit(argv);
                break;
        }
    }

    if (optind < argc) {
        print_usage_and_exit(argv);
    }

    our_pid = getpid();
    tty_name = ttyname(STDIN_FILENO);

    struct proc_info *info;
    size_t num_pids;
    if (read_procfs_info(&info, &num_pids, 0)) {
        perror("read_procfs_info");
        return 1;
    }

    if (print_extra) {
        printf("%.1s %-8s %5s %5s %-12s %s\n", "S", "UID", "PID", "PPID", "TTY", "CMD");
    } else {
        printf("%5s %-12s %s\n", "PID", "TTY", "CMD");
    }

    for (size_t i = 0; i < num_pids; i++) {
        if (print_all || strcmp(info[i].tty, tty_name) == 0) {
            if (print_extra) {
                struct passwd *p = getpwuid(info[i].uid);
                printf("%.1s %-8s %5d %5d %-12s %s\n", info[i].state, p ? p->pw_name : "unknown", info[i].pid, info[i].ppid, info[i].tty,
                       info[i].name);
            } else {
                printf("%5d %-12s %s\n", info[i].pid, info[i].tty, info[i].name);
            }
        }
    }

    free_procfs_info(info);
    return 0;
}