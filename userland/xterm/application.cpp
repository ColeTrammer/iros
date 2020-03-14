#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>

#include <kernel/hal/input.h>

#include "application.h"
#include "tty.h"

extern volatile sig_atomic_t g_pid_dead;

Application::Application() {
    for (int i = 0; i < 4; i++) {
        Terminal t;
        m_terminals.add(move(t));
    }
    switch_to(0);
}

Application::~Application() {}

int Application::current_mfd() const {
    return current_tty().mfd();
}

void Application::reset_tty_with_pid(int pid) {
    auto* tty = m_terminals.first_match([pid](const auto& terminal) {
        return terminal.pid() == pid;
    });
    if (tty) {
        tty->reset();
        m_current_tty = -1;
        for (int i = 0; i < m_terminals.size(); i++) {
            if (m_terminals[i].is_loaded()) {
                switch_to(i);
                return;
            }
        }
        switch_to(0);
    }
}

void Application::switch_to(int tty_number) {
    if (tty_number == m_current_tty) {
        return;
    }

    if (m_current_tty != -1) {
        current_tty().save();
    }
    assert(tty_number >= 0 && tty_number < m_terminals.size());
    m_current_tty = tty_number;
    if (!current_tty().is_loaded()) {
        current_tty().load(m_container);
    }
    current_tty().switch_to();
}

int Application::run() {
    int kfd = open("/dev/keyboard", O_RDONLY);
    assert(kfd != -1);

    int mouse_fd = open("/dev/mouse", O_RDONLY);
    assert(mouse_fd != -1);

    char buf[4096];
    mouse_event mouse_event;
    key_event event;

    fd_set set;

    sigset_t sigmask;
    sigprocmask(0, nullptr, &sigmask);
    sigdelset(&sigmask, SIGCHLD);

    for (;;) {
        int mfd = current_mfd();
        assert(mfd != -1);
        auto& tty = current_tty().tty();

        FD_ZERO(&set);
        FD_SET(mouse_fd, &set);
        FD_SET(kfd, &set);
        FD_SET(mfd, &set);

        int ret = pselect(FD_SETSIZE, &set, nullptr, nullptr, nullptr, &sigmask);
        assert(ret != 0);
        if (ret == -1) {
            if (errno == EINTR) {
                if (g_pid_dead != -1) {
                    reset_tty_with_pid(g_pid_dead);
                    g_pid_dead = -1;
                }
                continue;
            }

            perror("select");
            return 1;
        }

        if (FD_ISSET(mouse_fd, &set)) {
            if (read(mouse_fd, &mouse_event, sizeof(struct mouse_event)) == sizeof(struct mouse_event)) {
                if (mouse_event.scroll_state == SCROLL_UP) {
                    tty.scroll_up();
                } else if (mouse_event.scroll_state == SCROLL_DOWN) {
                    tty.scroll_down();
                }
            }
        }

        if (FD_ISSET(kfd, &set)) {
            if (read(kfd, &event, sizeof(key_event)) == sizeof(key_event)) {
                if (event.flags & KEY_DOWN) {
                    if (event.flags & KEY_SHIFT_ON && !(event.flags & KEY_ALT_ON) && !(event.flags & KEY_CONTROL_ON)) {
                        if (event.key == KEY_HOME) {
                            tty.scroll_to_top();
                            continue;
                        } else if (event.key == KEY_END) {
                            tty.scroll_to_bottom();
                            continue;
                        }
                    }
                    if (event.flags & KEY_CONTROL_ON) {
                        event.ascii &= 0x1F;
                        switch (event.key) {
                            case KEY_BACKSPACE: {
                                // NOTE: no one knows what this should be...
                                char c = 'W' & 0x1F;
                                write(mfd, &c, 1);
                                break;
                            }
                            case KEY_DELETE:
                                write(mfd, "\033[3;5~", 6);
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
                            case KEY_1:
                                switch_to(0);
                                break;
                            case KEY_2:
                                switch_to(1);
                                break;
                            case KEY_3:
                                switch_to(2);
                                break;
                            case KEY_4:
                                switch_to(3);
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
        }

        if (FD_ISSET(mfd, &set)) {
            ssize_t bytes;
            if ((bytes = read(mfd, buf, 4096)) > 0) {
                for (int i = 0; i < bytes; i++) {
                    tty.on_char(buf[i]);
                }
            }

            if (bytes == -1) {
                perror("master read");
                return 1;
            }

            tty.refresh();
        }
    }
}