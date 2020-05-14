#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <liim/string.h>
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

bool Application::handle_mouse_event(mouse_event event) {
    auto& tty = current_tty().tty();
    if (event.scroll_state == SCROLL_UP) {
        tty.scroll_up();
    } else if (event.scroll_state == SCROLL_DOWN) {
        tty.scroll_down();
    }

    return false;
}

bool Application::handle_keyboard_event(key_event event) {
    int mfd = current_mfd();
    auto& tty = current_tty().tty();
    if (!(event.flags & KEY_DOWN)) {
        return false;
    }

    if (event.flags & KEY_SHIFT_ON && !(event.flags & KEY_ALT_ON) && !(event.flags & KEY_CONTROL_ON)) {
        if (event.key == KEY_HOME) {
            tty.scroll_to_top();
            return true;
        } else if (event.key == KEY_END) {
            tty.scroll_to_bottom();
            return true;
        }
    }

    int modifiers = 1;
    if (event.flags & KEY_CONTROL_ON) {
        modifiers += 4;
    }
    if (event.flags & KEY_ALT_ON) {
        modifiers += 2;
    }
    if (event.flags & KEY_SHIFT_ON) {
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

    switch (event.key) {
        case KEY_CURSOR_UP:
            send_xterm_escape('A', modifiers);
            return false;
        case KEY_CURSOR_DOWN:
            send_xterm_escape('B', modifiers);
            return false;
        case KEY_CURSOR_RIGHT:
            send_xterm_escape('C', modifiers);
            return false;
        case KEY_CURSOR_LEFT:
            send_xterm_escape('D', modifiers);
            return false;
        case KEY_END:
            send_xterm_escape('F', modifiers);
            return false;
        case KEY_HOME:
            send_xterm_escape('H', modifiers);
            return false;
        case KEY_INSERT:
            send_vt_escape(2, modifiers);
            return false;
        case KEY_DELETE:
            send_vt_escape(3, modifiers);
            return false;
        case KEY_PAGE_UP:
            send_vt_escape(5, modifiers);
            return false;
        case KEY_PAGE_DOWN:
            send_vt_escape(6, modifiers);
            return false;
        case KEY_F1:
            send_vt_escape(11, modifiers);
            return false;
        case KEY_F2:
            send_vt_escape(12, modifiers);
            return false;
        case KEY_F3:
            send_vt_escape(13, modifiers);
            return false;
        case KEY_F4:
            send_vt_escape(14, modifiers);
            return false;
        case KEY_F5:
            send_vt_escape(15, modifiers);
            return false;
        case KEY_F6:
            send_vt_escape(17, modifiers);
            return false;
        // case KEY_F7:
        //     send_vt_escape(18, modifiers);
        //     return false;
        case KEY_F8:
            send_vt_escape(19, modifiers);
            return false;
        case KEY_F9:
            send_vt_escape(20, modifiers);
            return false;
        case KEY_F10:
            send_vt_escape(21, modifiers);
            return false;
        case KEY_F11:
            send_vt_escape(23, modifiers);
            return false;
        case KEY_F12:
            send_vt_escape(24, modifiers);
            return false;
        case KEY_1:
            if (modifiers & KEY_CONTROL_ON) {
                switch_to(0);
                return false;
            }
        case KEY_2:
            if (modifiers & KEY_CONTROL_ON) {
                switch_to(1);
                return false;
            }
        case KEY_3:
            if (modifiers & KEY_CONTROL_ON) {
                switch_to(2);
                return false;
            }
        case KEY_4:
            if (modifiers & KEY_CONTROL_ON) {
                switch_to(3);
                return false;
            }
        default:
            break;
    }

    if (!event.ascii) {
        return false;
    }

    if (event.flags & KEY_CONTROL_ON) {
        event.ascii &= 0x1F;
    }

    write(mfd, &event.ascii, 1);
    return false;
}

int Application::run() {
#ifdef KERNEL_NO_GRAPHICS
    int kfd = open("/dev/keyboard", O_RDONLY);
    assert(kfd != -1);

    int mouse_fd = open("/dev/mouse", O_RDONLY);
    assert(mouse_fd != -1);

    mouse_event mouse_event;
    key_event event;
#endif /* KERNEL_NO_GRAPHICS */

    char buf[4096];

    fd_set set;

    sigset_t sigmask;
    sigprocmask(0, nullptr, &sigmask);
    sigdelset(&sigmask, SIGCHLD);

    for (;;) {
        int mfd = current_mfd();
        assert(mfd != -1);
        auto& tty = current_tty().tty();

        FD_ZERO(&set);
        FD_SET(mfd, &set);
#ifdef KERNEL_NO_GRAPHICS
        FD_SET(mouse_fd, &set);
        FD_SET(kfd, &set);
#else
        FD_SET(m_container.connection().fd(), &set);
#endif /* KERNEL_NO_GRAPHICS */

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

#ifdef KERNEL_NO_GRAPHICS
        if (FD_ISSET(mouse_fd, &set)) {
            if (read(mouse_fd, &mouse_event, sizeof(struct mouse_event)) == sizeof(struct mouse_event)) {
                if (handle_mouse_event(mouse_event)) {
                    continue;
                }
            }
        }

        if (FD_ISSET(kfd, &set)) {
            if (read(kfd, &event, sizeof(key_event)) == sizeof(key_event)) {
                if (handle_keyboard_event(event)) {
                    continue;
                }
            }
        }
#else
        if (FD_ISSET(m_container.connection().fd(), &set)) {
            ssize_t ret = read(m_container.connection().fd(), buf, sizeof(buf));
            if (ret > 0) {
                WindowServer::Message* message = reinterpret_cast<WindowServer::Message*>(buf);
                switch (message->type) {
                    case WindowServer::Message::Type::KeyEventMessage:
                        if (handle_keyboard_event(message->data.key_event_message.event)) {
                            continue;
                        }
                        break;
                    case WindowServer::Message::Type::MouseEventMessage:
                        if (handle_mouse_event(message->data.mouse_event_message.event)) {
                            continue;
                        }
                        break;
                    default:
                        assert(false);
                        break;
                }
            }
        }
#endif /* KERNEL_NO_GRAPHICS */

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
