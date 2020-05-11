#include <assert.h>
#include <procinfo.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#define PID_WIDTH     5
#define PID_PREC      0
#define PID_FLAGS     ""
#define PID_SPECIFIER "d"
#define PID_STRING    "PID"

#define USER_WIDTH     8
#define USER_PREC      8
#define USER_FLAGS     "-"
#define USER_SPECIFIER "s"
#define USER_STRING    "USER"

#define PRIORITY_WIDTH     3
#define PRIORITY_PREC      0
#define PRIORITY_FLAGS     ""
#define PRIORITY_SPECIFIER "d"
#define PRIORITY_STRING    "PR"

#define NICE_WIDTH     3
#define NICE_PREC      0
#define NICE_FLAGS     ""
#define NICE_SPECIFIER "d"
#define NICE_STRING    "NI"

#define VIRTUAL_MEM_WIDTH     8
#define VIRTUAL_MEM_PREC      0
#define VIRTUAL_MEM_FLAGS     ""
#define VIRTUAL_MEM_SPECIFIER "ld"
#define VIRTUAL_MEM_STRING    "VIRT"

#define RESIDENT_MEM_WIDTH     8
#define RESIDENT_MEM_PREC      0
#define RESIDENT_MEM_FLAGS     ""
#define RESIDENT_MEM_SPECIFIER "ld"
#define RESIDENT_MEM_STRING    "RES"

#define STATUS_WIDTH     1
#define STATUS_PREC      1
#define STATUS_FLAGS     ""
#define STATUS_SPECIFIER "s"
#define STATUS_STRING    "S"

#define CPU_WIDTH     5
#define CPU_PREC      1
#define CPU_FLAGS     ""
#define CPU_SPECIFIER "f"
#define CPU_STRING    "%CPU"

#define MEM_WIDTH     5
#define MEM_PREC      1
#define MEM_FLAGS     ""
#define MEM_SPECIFIER "f"
#define MEM_STRING    "%MEM"

#define TIME_WIDTH     9
#define TIME_PREC      9
#define TIME_FLAGS     ""
#define TIME_SPECIFIER "s"
#define TIME_STRING    "TIME+"

#define NAME_WIDTH                                                                                                                     \
    (win_size.ws_col - (PID_WIDTH + USER_WIDTH + PRIORITY_WIDTH + NICE_WIDTH + VIRTUAL_MEM_WIDTH + RESIDENT_MEM_WIDTH + STATUS_WIDTH + \
                        CPU_WIDTH + MEM_WIDTH + TIME_WIDTH + 10))
#define NAME_PREC      NAME_WIDTH
#define NAME_FLAGS     "-"
#define NAME_SPECIFIER "s"
#define NAME_STRING    "COMMAND"

#define FORMAT_STRING_HEADER                                                                                                             \
    "%" PID_FLAGS "*.*s %" USER_FLAGS "*.*s %" VIRTUAL_MEM_FLAGS "*.*s %" PRIORITY_FLAGS "*.*s %" NICE_FLAGS "*.*s %" RESIDENT_MEM_FLAGS \
    "*.*s %" STATUS_FLAGS "*.*s %" CPU_FLAGS "*.*s %" MEM_FLAGS "*.*s %" TIME_FLAGS "*.*s %" NAME_FLAGS "*.*s\n"
#define FORMAT_STRING_ROW                                                                                                               \
    "%" PID_FLAGS "*.*" PID_SPECIFIER " %" USER_FLAGS "*.*" USER_SPECIFIER " %" PRIORITY_FLAGS "*.*" PRIORITY_SPECIFIER " %" NICE_FLAGS \
    "*.*" NICE_SPECIFIER " %" VIRTUAL_MEM_FLAGS "*.*" VIRTUAL_MEM_SPECIFIER " %" RESIDENT_MEM_FLAGS "*.*" RESIDENT_MEM_SPECIFIER        \
    " %" STATUS_FLAGS "*.*" STATUS_SPECIFIER " %" CPU_FLAGS "*.*" CPU_SPECIFIER " %" MEM_FLAGS "*.*" MEM_SPECIFIER " %" TIME_FLAGS      \
    "*.*" TIME_SPECIFIER " %" NAME_FLAGS "*.*" NAME_SPECIFIER "\n"

struct proc_summary {
    int tasks_total;
    int tasks_running;
    int tasks_sleeping;
};

static struct winsize win_size;
static struct termios tty_info;

static struct proc_global_info *prev_global_data;
static struct proc_global_info current_global_info;
static struct proc_info *prev_data;
static size_t prev_data_num;

static void reset_cursor() {
    printf("\033[1;1H");
}

static void enable_cursor() {
    printf("\033[?25h");
}

static void disable_cursor() {
    printf("\033[?25l");
}

static size_t display_header(struct proc_summary *summary) {
    int written = printf("Tasks: \033[1;97m%3d\033[0m total, \033[1;97m%3d\033[0m running, \033[1;97m%3d\033[0m sleeping",
                         summary->tasks_total, summary->tasks_running, summary->tasks_sleeping);
    printf("%*s\n", win_size.ws_col - written, "");

    printf("%*s\n", win_size.ws_col, "");
    printf("\033[7m" FORMAT_STRING_HEADER "\033[0m", PID_WIDTH, PID_WIDTH, PID_STRING, USER_WIDTH, USER_WIDTH, USER_STRING, PRIORITY_WIDTH,
           PRIORITY_WIDTH, PRIORITY_STRING, NICE_WIDTH, NICE_WIDTH, NICE_STRING, VIRTUAL_MEM_WIDTH, VIRTUAL_MEM_WIDTH, VIRTUAL_MEM_STRING,
           RESIDENT_MEM_WIDTH, RESIDENT_MEM_WIDTH, RESIDENT_MEM_STRING, STATUS_WIDTH, STATUS_WIDTH, STATUS_STRING, CPU_WIDTH, CPU_WIDTH,
           CPU_STRING, MEM_WIDTH, MEM_WIDTH, MEM_STRING, TIME_WIDTH, TIME_WIDTH, TIME_STRING, NAME_WIDTH, NAME_WIDTH, NAME_STRING);
    return 3;
}

static int prev_process_ticks(pid_t pid, uint64_t *out_ticks) {
    assert(prev_data);
    for (size_t i = 0; i < prev_data_num; i++) {
        if (prev_data[i].pid == pid) {
            *out_ticks = prev_data[i].kernel_ticks + prev_data[i].user_ticks;
            return 0;
        }
    }

    return 1;
}

static double compute_cpu_usage(const struct proc_info *info) {
    uint64_t total_process_ticks = info->user_ticks + info->kernel_ticks;
    uint64_t total_ticks = current_global_info.idle_ticks + current_global_info.user_ticks + current_global_info.kernel_ticks;

    if (!prev_data || !prev_global_data) {
        return (double) total_process_ticks / (double) total_ticks * 100;
    } else {
        uint64_t prev_ticks = 0.0;
        prev_process_ticks(info->pid, &prev_ticks);

        uint64_t d_total_ticks = total_ticks - prev_global_data->idle_ticks - prev_global_data->user_ticks - prev_global_data->kernel_ticks;
        uint64_t d_process_ticks = total_process_ticks - prev_ticks;
        return (double) d_process_ticks / (double) d_total_ticks * 100;
    }
}

static void display_row(struct proc_info *info) {
    struct passwd *user = getpwuid(info->uid);
    char *user_string = user ? user->pw_name : "unknown";

    double cpu_percent = compute_cpu_usage(info);
    double mem_percent = (double) info->resident_memory / (double) current_global_info.total_memory * 100;

    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) < 0) {
        perror("clock_gettime");
        exit(1);
    }
    struct timespec delta = { .tv_sec = now.tv_sec - info->start_time.tv_sec, .tv_nsec = now.tv_nsec - info->start_time.tv_nsec };
    double seconds = delta.tv_sec + delta.tv_nsec / 1000000000.0;
    long int_seconds = (int) seconds;
    seconds -= int_seconds;
    seconds *= 100;

    long minutes = int_seconds / 60;
    int_seconds %= 60;

    char time_string[80];
    snprintf(time_string, sizeof(time_string) - 1, "%ld:%02ld.%02d", minutes, int_seconds, (int) seconds);

    printf(FORMAT_STRING_ROW, PID_WIDTH, PID_PREC, info->pid, USER_WIDTH, USER_PREC, user_string, PRIORITY_WIDTH, PRIORITY_PREC,
           info->priority, NICE_WIDTH, NICE_PREC, info->nice, VIRTUAL_MEM_WIDTH, VIRTUAL_MEM_PREC, info->virtual_memory, RESIDENT_MEM_WIDTH,
           RESIDENT_MEM_PREC, info->resident_memory, STATUS_WIDTH, STATUS_PREC, info->state, CPU_WIDTH, CPU_PREC, cpu_percent, MEM_WIDTH,
           MEM_PREC, mem_percent, TIME_WIDTH, TIME_PREC, time_string, NAME_WIDTH, NAME_PREC, info->name);
}

static void summarize_data(struct proc_info *info, size_t num_pids, struct proc_summary *summary) {
    int total = 0;
    int running = 0;
    int sleeping = 0;

    for (size_t i = 0; i < num_pids; i++) {
        struct proc_info *proc = info + i;
        if (proc->pid != 1) {
            total++;
            if (proc->state[0] == 'R' || proc->state[0] == 'U') {
                running++;
            } else if (proc->state[0] == 'W') {
                sleeping++;
            }
        }
    }

    summary->tasks_total = total;
    summary->tasks_running = running;
    summary->tasks_sleeping = sleeping;
}

static void display(struct proc_info *info, size_t num_pids) {
    reset_cursor();

    struct proc_summary summary;
    summarize_data(info, num_pids, &summary);
    size_t header_rows = display_header(&summary);

    size_t num_rows_available = (size_t) win_size.ws_row - header_rows;
    size_t num_rows_to_display = MIN(num_pids, num_rows_available);
    for (size_t i = 0; i < num_rows_to_display; i++) {
        display_row(info + i);
    }

    for (size_t i = num_rows_to_display; i < num_rows_available - 1; i++) {
        printf("%*s\n", win_size.ws_col, "");
    }
}

static int proc_info_compar(const void *_i1, const void *_i2) {
    const struct proc_info *i1 = _i1;
    const struct proc_info *i2 = _i2;
    double c1 = compute_cpu_usage(i1);
    double c2 = compute_cpu_usage(i2);
    return c1 < c2 ? 1 : c1 == c2 ? 0 : -1;
}

static void update() {
    struct proc_info *info;
    size_t num_pids;
    if (read_procfs_info(&info, &num_pids, READ_PROCFS_SCHED)) {
        perror("read_procfs_info");
        exit(1);
    }

    if (read_procfs_global_info(&current_global_info, READ_PROCFS_GLOBAL_MEMINFO | READ_PROCFS_GLOBAL_SCHED)) {
        perror("read_procfs_global_info");
        exit(1);
    }

    qsort(info, num_pids, sizeof(struct proc_info), proc_info_compar);
    display(info, num_pids);

    if (prev_data) {
        free_procfs_info(prev_data);
    }
    prev_data = info;
    prev_data_num = num_pids;

    if (!prev_global_data) {
        prev_global_data = malloc(sizeof(struct proc_global_info));
    }
    *prev_global_data = current_global_info;
}

static void on_input(char *buffer, size_t amount_read) {
    if (amount_read != 1) {
        return;
    }

    if (*buffer == 'q' || *buffer == ('c' & 0x1f)) {
        exit(0);
    }
}

static void cleanup() {
    putchar('\n');
    enable_cursor();
    tcsetattr(STDOUT_FILENO, TCIOFLUSH, &tty_info);
    free_procfs_info(prev_data);
    free(prev_global_data);
}

int main() {
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win_size)) {
        perror("ioctl(TIOCGWINSZ)");
        return 1;
    }

    if (tcgetattr(STDOUT_FILENO, &tty_info)) {
        perror("tcgetattr");
        return 1;
    }

    struct termios new_tty_info = tty_info;
    new_tty_info.c_iflag &= ~(IXON);
    new_tty_info.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    new_tty_info.c_cc[VMIN] = 0;
    new_tty_info.c_cc[VTIME] = 0;

    if (tcsetattr(STDOUT_FILENO, TCIOFLUSH, &new_tty_info)) {
        perror("tcsetattr");
        return 1;
    }

    atexit(cleanup);
    disable_cursor();

    update();

    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    struct timespec timeout = { .tv_sec = 2, .tv_nsec = 0 };

    char buf[BUFSIZ];
    while (select(FD_SETSIZE, &set, NULL, NULL, &timeout) >= 0) {
        if (FD_ISSET(STDIN_FILENO, &set)) {
            ssize_t ret = read(STDIN_FILENO, buf, sizeof(buf));
            if (ret < 0) {
                perror("read");
                return 1;
            }
            on_input(buf, (size_t) ret);
        }

        update();
        FD_SET(STDIN_FILENO, &set);
    }

    return 0;
}
