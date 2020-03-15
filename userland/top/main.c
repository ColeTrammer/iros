#include <pwd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/os_2.h>
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

#define NAME_WIDTH     (win_size.ws_col - (PID_WIDTH + USER_WIDTH + 2))
#define NAME_PREC      NAME_WIDTH
#define NAME_FLAGS     "-"
#define NAME_SPECIFIER "s"
#define NAME_STRING    "COMMAND"

#define FORMAT_STRING_HEADER "%" PID_FLAGS "*.*s %" USER_FLAGS "*.*s %" NAME_FLAGS "*.*s\n"
#define FORMAT_STRING_ROW    "%" PID_FLAGS "*.*" PID_SPECIFIER " %" USER_FLAGS "*.*" USER_SPECIFIER " %" NAME_FLAGS "*.*" NAME_SPECIFIER "\n"

static struct winsize win_size;
static struct termios tty_info;

static void reset_cursor() {
    printf("\033[1;1H");
}

static void enable_cursor() {
    printf("\033[?25h");
}

static void disable_cursor() {
    printf("\033[?25l");
}

static size_t display_header() {
    printf("\033[7m" FORMAT_STRING_HEADER "\033[0m", PID_WIDTH, PID_WIDTH, PID_STRING, USER_WIDTH, USER_WIDTH, USER_STRING, NAME_WIDTH,
           NAME_WIDTH, NAME_STRING);
    return 1;
}

static void display_row(struct proc_info *info) {
    struct passwd *user = getpwuid(info->uid);
    char *user_string = user ? user->pw_name : "unknown";
    printf(FORMAT_STRING_ROW, PID_WIDTH, PID_PREC, info->pid, USER_WIDTH, USER_PREC, user_string, NAME_WIDTH, NAME_PREC, info->name);
}

static void display(struct proc_info *info, size_t num_pids) {
    reset_cursor();
    size_t header_rows = display_header();

    size_t num_rows_available = (size_t) win_size.ws_row - header_rows;
    size_t num_rows_to_display = MIN(num_pids, num_rows_available);
    for (size_t i = 0; i < num_rows_to_display; i++) {
        display_row(info + i);
    }

    for (size_t i = num_rows_to_display; i < num_rows_available; i++) {
        char buffer[255];
        snprintf(buffer, sizeof(buffer) - 1, "%%%us%s", win_size.ws_col, i == num_rows_available - 1 ? "" : "\n");
        printf(buffer, "");
    }
}

static void update() {
    struct proc_info *info;
    size_t num_pids;
    if (read_procfs_info(&info, &num_pids, 0)) {
        perror("read_procfs_info");
        exit(1);
    }

    display(info, num_pids);
    free_procfs_info(info);
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

    struct timespec timeout = { .tv_sec = 1, .tv_nsec = 0 };

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