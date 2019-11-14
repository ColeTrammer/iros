#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <kernel/hal/input.h>

#include "tty.h"
#include "vga_buffer.h"

int main() 
{
    close(0);
    close(1);
    close(2);

    VgaBuffer vga_buffer("/dev/fb0");
    TTY tty(vga_buffer);

    int mfd = posix_openpt(O_RDWR);
    assert(mfd != -1);

    pid_t pid = fork();
    if (pid == 0) {
        int sfd = open(ptsname(mfd), O_RDWR);
        assert(sfd != -1);

        setpgid(0, 0);
        signal(SIGTTOU, SIG_IGN);
        tcsetpgrp(mfd, getpid());
        ioctl(mfd, TIOSCTTY);
        signal(SIGTTOU, SIG_DFL);

        dup2(sfd, STDIN_FILENO);
        dup2(sfd, STDOUT_FILENO);
        dup2(sfd, STDERR_FILENO);

        close(sfd);
        close(mfd);

        execl("/bin/sh", "sh", NULL);
        _exit(127);
    } else if (pid == -1) {
        perror("fork");
        return -1;
    }

    setpgid(pid, pid);
    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(mfd, pid);
    signal(SIGTTOU, SIG_DFL);

    signal(SIGCHLD, [](auto) {
        int status;
        waitpid(-1, &status, WNOHANG);
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            exit(0);
        }
    });

    int kfd = open("/dev/keyboard", O_RDONLY);
    assert(kfd != -1);

    int mouse_fd = open("/dev/mouse", O_RDONLY);
    assert(mouse_fd != -1);

    for (;;) {
        mouse_event mouse_event;
        while (read(mouse_fd, &mouse_event, sizeof(struct mouse_event)) == sizeof(struct mouse_event)) {
            if (mouse_event.scroll_state == SCROLL_UP) {
                tty.scroll_up();
            } else if (mouse_event.scroll_state == SCROLL_DOWN) {
                tty.scroll_down();
            }
        }

        key_event event;
        while (read(kfd, &event, sizeof(key_event)) == sizeof(key_event)) {
            if (event.flags & KEY_DOWN) {
                if (event.flags & KEY_CONTROL_ON) {
                    event.ascii &= 0x1F;
                    switch (event.key) {
                    case KEY_DELETE:
                        write(mfd, "\033[3~", 4);
                        break;
                    case KEY_PAGE_UP:
                        write(mfd, "\033[5~", 4);
                        break;
                    case KEY_PAGE_DOWN:
                        write(mfd, "\033[6~", 4);
                        break;
                    case KEY_CURSOR_UP:
                        write(mfd, "\033[1;5A", 6);
                        break;
                    case KEY_CURSOR_DOWN:
                        write(mfd, "\033[1;5B", 6);
                        break;
                    case KEY_CURSOR_RIGHT:
                        write(mfd, "\033[1;5C", 6);
                        break;
                    case KEY_CURSOR_LEFT:
                        write(mfd, "\033[1;5D", 6);
                        break;
                    case KEY_HOME:
                        write(mfd, "\033[1;5H", 6);
                        break;
                    case KEY_END:
                        write(mfd, "\033[1;5F", 6);
                        break;
                    default:
                        if (event.ascii != '\0') {
                            write(mfd, &event.ascii, 1);
                        }
                        break;
                    }
                } else {
                    switch (event.key) {
                    case KEY_DELETE:
                        write(mfd, "\033[3~", 4);
                        break;
                    case KEY_PAGE_UP:
                        write(mfd, "\033[5~", 4);
                        break;
                    case KEY_PAGE_DOWN:
                        write(mfd, "\033[6~", 4);
                        break;
                    case KEY_CURSOR_UP:
                        write(mfd, "\033[A", 3);
                        break;
                    case KEY_CURSOR_DOWN:
                        write(mfd, "\033[B", 3);
                        break;
                    case KEY_CURSOR_RIGHT:
                        write(mfd, "\033[C", 3);
                        break;
                    case KEY_CURSOR_LEFT:
                        write(mfd, "\033[D", 3);
                        break;
                    case KEY_HOME:
                        write(mfd, "\033[H", 3);
                        break;
                    case KEY_END:
                        write(mfd, "\033[F", 3);
                        break;
                    default:
                        if (event.ascii != '\0') {
                            write(mfd, &event.ascii, 1);
                        }
                        break;
                    }
                }
            }
        }

        char buf[4096];
        ssize_t bytes;
        while ((bytes = read(mfd, buf, 4096)) > 0) {
            for (int i = 0; i < bytes; i++) {
                tty.on_char(buf[i]);
            }
        }

        if (bytes == -1) {
            break;
        }
    }

    return 0;
}