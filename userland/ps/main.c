#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

static bool print_all;
static bool print_extra;

static pid_t our_pid;
static char *tty_name;

static int scandir_filter(const struct dirent *ent) {
    for (size_t i = 0; ent->d_name[i] != '\0'; i++) {
        if (!isdigit(ent->d_name[i])) {
            return 0;
        }
    }

    return atoi(ent->d_name) != our_pid;
}

static int sort_by_pid(const struct dirent **a, const struct dirent **b) {
    int a_pid = atoi((*a)->d_name);
    int b_pid = atoi((*b)->d_name);
    if (a_pid < b_pid) {
        return -1;
    } else if (a_pid == b_pid) {
        return 0;
    } else {
        return 1;
    }
}

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

    struct dirent **pids;
    int num_pids;
    if ((num_pids = scandir("/proc", &pids, scandir_filter, sort_by_pid)) == -1) {
        perror("scandir(\"/proc\")");
        return 1;
    }

    struct proc_info {
        char name[64];
        pid_t pid;
        uid_t uid;
        gid_t gid;
        pid_t ppid;
        mode_t umask;
        uid_t euid;
        gid_t egid;
        pid_t pgid;
        pid_t sid;
        char tty[64];
    };

    struct proc_info *info = malloc(num_pids * sizeof(struct proc_info));

    for (int i = 0; i < num_pids; i++) {
        struct dirent *dirent = pids[i];

        char path_buf[1024];
        snprintf(path_buf, sizeof(path_buf) - 1, "/proc/%s/status", dirent->d_name);

        FILE *status = fopen(path_buf, "r");
        if (!status) {
            perror("fopen");
            return 1;
        }

#define _(x) "" #x
#define READ_PROCFS_STRING_FIELD(name, convert_spec)                                                                 \
    do {                                                                                                             \
        char field_name[64];                                                                                         \
        if (fscanf(status, "%64[^:]: " convert_spec, field_name, info[i].name) != 2) {                               \
            fprintf(stderr, "fscanf: failed to read field `%s`\n", _(name));                                         \
            return 1;                                                                                                \
        }                                                                                                            \
        if (strcasecmp(field_name, _(name)) != 0) {                                                                  \
            fprintf(stderr, "Error: expected `%s` but got `%s` when reading `%s`\n", _(name), field_name, path_buf); \
            return 1;                                                                                                \
        }                                                                                                            \
    } while (0)
#define READ_PROCFS_FIELD(name, convert_spec)                                                                        \
    do {                                                                                                             \
        char field_name[64];                                                                                         \
        if (fscanf(status, "%64[^:]: " convert_spec, field_name, &info[i].name) != 2) {                              \
            fprintf(stderr, "fscanf: failed to read field `%s`\n", _(name));                                         \
            return 1;                                                                                                \
        }                                                                                                            \
        if (strcasecmp(field_name, _(name)) != 0) {                                                                  \
            fprintf(stderr, "Error: expected `%s` but got `%s` when reading `%s`\n", _(name), field_name, path_buf); \
            return 1;                                                                                                \
        }                                                                                                            \
    } while (0)

        READ_PROCFS_STRING_FIELD(name, "%64s");
        READ_PROCFS_FIELD(pid, "%d");
        READ_PROCFS_FIELD(uid, "%hu");
        READ_PROCFS_FIELD(gid, "%hu");
        READ_PROCFS_FIELD(ppid, "%d");
        READ_PROCFS_FIELD(umask, "%o");
        READ_PROCFS_FIELD(euid, "%hu");
        READ_PROCFS_FIELD(egid, "%hu");
        READ_PROCFS_FIELD(pgid, "%d");
        READ_PROCFS_FIELD(sid, "%d");
        READ_PROCFS_STRING_FIELD(tty, "%64s");

        if (fclose(status)) {
            perror("fclose");
            return 1;
        }
    }

    if (print_extra) {
        printf("%-8s %5s %5s %-12s %s\n", "UID", "PID", "PPID", "TTY", "CMD");
    } else {
        printf("%5s %-12s %s\n", "PID", "TTY", "CMD");
    }

    for (int i = 0; i < num_pids; i++) {
        if (print_all || strcmp(info[i].tty, tty_name) == 0) {
            if (print_extra) {
                struct passwd *p = getpwuid(info[i].uid);
                printf("%-8s %5d %5d %-12s %s\n", p ? p->pw_name : "unknown", info[i].pid, info[i].ppid, info[i].tty, info[i].name);
            } else {
                printf("%5d %-12s %s\n", info[i].pid, info[i].tty, info[i].name);
            }
        }
    }

    free(info);
    return 0;
}