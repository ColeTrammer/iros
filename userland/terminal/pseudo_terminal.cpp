#include <fcntl.h>
#include <liim/string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "pseudo_terminal.h"

PsuedoTerminal::PsuedoTerminal() {
    m_master_fd = posix_openpt(O_RDWR);
    if (m_master_fd == -1) {
        perror("terminal: posix_openpt");
        exit(1);
    }

    winsize ws;
    ws.ws_row = m_rows;
    ws.ws_col = m_cols;
    ioctl(m_master_fd, TIOCSWINSZ, &ws);

    m_child_pid = fork();
    if (m_child_pid < 0) {
        perror("terminal: fork");
        exit(1);
    }

    if (m_child_pid == 0) {
        int slave_fd = open(ptsname(m_master_fd), O_RDWR);
        if (slave_fd == -1) {
            perror("terminal (fork): open");
            _exit(1);
        }

        if (setsid() < 0) {
            perror("terminal (fork): setsid");
            _exit(1);
        }

        if (tcsetpgrp(m_master_fd, getpid())) {
            perror("terminal (fork): tcsetpgrp");
            _exit(1);
        }

        if (ioctl(m_master_fd, TIOSCTTY)) {
            perror("terminal (fork): ioctl(TIOSCTTY)");
            _exit(1);
        }

        signal(SIGTTOU, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGWINCH, SIG_DFL);

        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);

        close(slave_fd);
        close(m_master_fd);

        putenv((char*) "TERM=xterm");
        execl("/bin/sh", "sh", NULL);
        _exit(127);
    }

    tcsetpgrp(m_master_fd, m_child_pid);
}

PsuedoTerminal::~PsuedoTerminal() {
    kill(m_child_pid, SIGHUP);
    close(m_master_fd);
}

void PsuedoTerminal::handle_key_event(key key, int flags, char ascii) {
    int mfd = master_fd();

    if (!(flags & KEY_DOWN)) {
        return;
    }

    int modifiers = 1;
    if (flags & KEY_CONTROL_ON) {
        modifiers += 4;
    }
    if (flags & KEY_ALT_ON) {
        modifiers += 2;
    }
    if (flags & KEY_SHIFT_ON) {
        modifiers += 1;
    }

    auto send_xterm_escape = [&](char ch, int modifiers) {
        String seq;
        if (modifiers == 1) {
            seq = String::format("\033[%c", ch);
        } else {
            seq = String::format("\033[1;%d%c", modifiers, ch);
        }
        write(mfd, seq.string(), seq.size());
    };

    auto send_vt_escape = [&](int key_num, int modifiers) {
        String seq;
        if (modifiers == 1) {
            seq = String::format("\033[%d~", key_num);
        } else {
            seq = String::format("\033[%d;%d~", key_num, modifiers);
        }
        write(mfd, seq.string(), seq.size());
    };

    switch (key) {
        case KEY_CURSOR_UP:
            send_xterm_escape('A', modifiers);
            return;
        case KEY_CURSOR_DOWN:
            send_xterm_escape('B', modifiers);
            return;
        case KEY_CURSOR_RIGHT:
            send_xterm_escape('C', modifiers);
            return;
        case KEY_CURSOR_LEFT:
            send_xterm_escape('D', modifiers);
            return;
        case KEY_END:
            send_xterm_escape('F', modifiers);
            return;
        case KEY_HOME:
            send_xterm_escape('H', modifiers);
            return;
        case KEY_INSERT:
            send_vt_escape(2, modifiers);
            return;
        case KEY_DELETE:
            send_vt_escape(3, modifiers);
            return;
        case KEY_PAGE_UP:
            send_vt_escape(5, modifiers);
            return;
        case KEY_PAGE_DOWN:
            send_vt_escape(6, modifiers);
            return;
        case KEY_F1:
            send_vt_escape(11, modifiers);
            return;
        case KEY_F2:
            send_vt_escape(12, modifiers);
            return;
        case KEY_F3:
            send_vt_escape(13, modifiers);
            return;
        case KEY_F4:
            send_vt_escape(14, modifiers);
            return;
        case KEY_F5:
            send_vt_escape(15, modifiers);
            return;
        case KEY_F6:
            send_vt_escape(17, modifiers);
            return;
        case KEY_F7:
            send_vt_escape(18, modifiers);
            return;
        case KEY_F8:
            send_vt_escape(19, modifiers);
            return;
        case KEY_F9:
            send_vt_escape(20, modifiers);
            return;
        case KEY_F10:
            send_vt_escape(21, modifiers);
            return;
        case KEY_F11:
            send_vt_escape(23, modifiers);
            return;
        case KEY_F12:
            send_vt_escape(24, modifiers);
            return;
        default:
            break;
    }

    if (!ascii) {
        return;
    }

    if (flags & KEY_CONTROL_ON) {
        if (key == KEY_BACKSPACE) {
            ascii = 'W' & 0x1F;
        } else {
            ascii &= 0x1F;
        }
    }

    write(mfd, &ascii, 1);
}

void PsuedoTerminal::set_size(int rows, int cols) {
    if (m_rows == rows && m_cols == cols) {
        return;
    }

    winsize ws;
    ws.ws_row = rows;
    ws.ws_col = cols;
    ioctl(m_master_fd, TIOCSWINSZ, &ws);
    kill(tcgetpgrp(m_master_fd), SIGWINCH);
}
