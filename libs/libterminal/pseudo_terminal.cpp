#include <fcntl.h>
#include <liim/string.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <terminal/pseudo_terminal.h>
#include <termios.h>
#include <unistd.h>

namespace Terminal {
PsuedoTerminal::PsuedoTerminal() {
    passwd* pwd = getpwuid(getuid());
    assert(pwd);

    m_master_fd = posix_openpt(O_RDWR | O_NONBLOCK);
    if (m_master_fd == -1) {
        perror("terminal: posix_openpt");
        exit(1);
    }

    if (grantpt(m_master_fd) < 0) {
        perror("terminal: grantpt");
        exit(1);
    }

    if (unlockpt(m_master_fd) < 0) {
        perror("terminal: unlockpt");
        exit(1);
    }

    winsize ws;
    memset(&ws, 0, sizeof(ws));
    ws.ws_row = m_rows;
    ws.ws_col = m_cols;
    if (ioctl(m_master_fd, TIOCSWINSZ, &ws) < 0) {
        perror("terminal: ioctl(TIOCSWINSZ)\n");
        exit(1);
    }

    m_child_pid = fork();
    if (m_child_pid < 0) {
        perror("terminal: fork");
        exit(1);
    }

    if (m_child_pid == 0) {
        char* slave_path = ptsname(m_master_fd);
        if (!slave_path) {
            perror("terminal (fork): ptsname");
            _exit(1);
        }

        int slave_fd = open(slave_path, O_RDWR);
        if (slave_fd == -1) {
            perror("terminal (fork): open");
            _exit(1);
        }

        if (setsid() < 0) {
            perror("terminal (fork): setsid");
            _exit(1);
        }

        if (ioctl(m_master_fd, TIOCSCTTY)) {
            perror("terminal (fork): ioctl(TIOSCTTY)");
            _exit(1);
        }

#ifdef __linux__
        struct termios old_termios;
        if (tcgetattr(slave_fd, &old_termios)) {
            perror("terminal (fork): tcgetattr");
            _exit(1);
        }

        old_termios.c_cc[VERASE] = '\b';
        if (tcsetattr(slave_fd, 0, &old_termios)) {
            perror("terminal (fork): tcsetattr");
            _exit(1);
        }
#endif

        signal(SIGTTOU, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGWINCH, SIG_DFL);

        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);

        close(slave_fd);
        close(m_master_fd);

        putenv((char*) "TERM=xterm-256color");
        execl(pwd->pw_shell, pwd->pw_shell, "-i", NULL);
        _exit(127);
    }
}

PsuedoTerminal::~PsuedoTerminal() {
    kill(m_child_pid, SIGHUP);
    close(m_master_fd);
}

void PsuedoTerminal::send_clipboard_contents(const String& contents) {
    if (m_bracketed_paste) {
        this->write(String::format("\033[200~%s\033[201~", contents.string()));
    } else {
        this->write(contents);
    }
}

void PsuedoTerminal::write(const String& contents) {
    assert(::write(m_master_fd, contents.string(), contents.size()) == static_cast<ssize_t>(contents.size()));
}

void PsuedoTerminal::handle_key_event(const App::KeyEvent& event) {
    if (!event.key_down() || event.generates_text()) {
        return;
    }

    int modifiers = 1;
    if (event.modifiers() & App::KeyModifier::Meta) {
        modifiers += 8;
    }
    if (event.modifiers() & App::KeyModifier::Control) {
        modifiers += 4;
    }
    if (event.modifiers() & App::KeyModifier::Alt) {
        modifiers += 2;
    }
    if (event.modifiers() & App::KeyModifier::Shift) {
        modifiers += 1;
    }

    auto send_application_escape = [&](char ch, int modifiers) {
        String seq;
        if (modifiers == 1) {
            seq = String::format("\033O%c", ch);
        } else {
            seq = String::format("\033[1;%d%c", modifiers, ch);
        }
        this->write(seq);
    };

    auto send_xterm_escape = [&](char ch, int modifiers) {
        String seq;
        if (modifiers == 1) {
            seq = String::format("\033[%c", ch);
        } else {
            seq = String::format("\033[1;%d%c", modifiers, ch);
        }
        this->write(seq);
    };

    auto send_vt_escape = [&](int key_num, int modifiers) {
        String seq;
        if (modifiers == 1) {
            seq = String::format("\033[%d~", key_num);
        } else {
            seq = String::format("\033[%d;%d~", key_num, modifiers);
        }
        this->write(seq);
    };

    switch (event.key()) {
        case App::Key::UpArrow:
            if (m_application_cursor_keys) {
                send_application_escape('A', modifiers);
            } else {
                send_xterm_escape('A', modifiers);
            }
            return;
        case App::Key::DownArrow:
            if (m_application_cursor_keys) {
                send_application_escape('B', modifiers);
            } else {
                send_xterm_escape('B', modifiers);
            }
            return;
        case App::Key::RightArrow:
            if (m_application_cursor_keys) {
                send_application_escape('C', modifiers);
            } else {
                send_xterm_escape('C', modifiers);
            }
            return;
        case App::Key::LeftArrow:
            if (m_application_cursor_keys) {
                send_application_escape('D', modifiers);
            } else {
                send_xterm_escape('D', modifiers);
            }
            return;
        case App::Key::End:
            if (m_application_cursor_keys) {
                send_application_escape('F', modifiers);
            } else {
                send_xterm_escape('F', modifiers);
            }
            return;
        case App::Key::Home:
            if (m_application_cursor_keys) {
                send_application_escape('H', modifiers);
            } else {
                send_xterm_escape('H', modifiers);
            }
            return;
        case App::Key::Tab:
            if (event.shift_down()) {
                send_xterm_escape('Z', 1);
            } else {
                this->write(String { '\t' });
            }
            return;
        case App::Key::Enter:
            this->write(String { '\r' });
            return;
        case App::Key::Backspace:
            if (event.control_down()) {
                this->write(String { 'W' & 0x1F });
            } else {
                this->write(String { '\x08' });
            }
            return;
        case App::Key::Insert:
            send_vt_escape(2, modifiers);
            return;
        case App::Key::Delete:
            send_vt_escape(3, modifiers);
            return;
        case App::Key::PageUp:
            send_vt_escape(5, modifiers);
            return;
        case App::Key::PageDown:
            send_vt_escape(6, modifiers);
            return;
        case App::Key::F1:
            send_vt_escape(11, modifiers);
            return;
        case App::Key::F2:
            send_vt_escape(12, modifiers);
            return;
        case App::Key::F3:
            send_vt_escape(13, modifiers);
            return;
        case App::Key::F4:
            send_vt_escape(14, modifiers);
            return;
        case App::Key::F5:
            send_vt_escape(15, modifiers);
            return;
        case App::Key::F6:
            send_vt_escape(17, modifiers);
            return;
        case App::Key::F7:
            send_vt_escape(18, modifiers);
            return;
        case App::Key::F8:
            send_vt_escape(19, modifiers);
            return;
        case App::Key::F9:
            send_vt_escape(20, modifiers);
            return;
        case App::Key::F10:
            send_vt_escape(21, modifiers);
            return;
        case App::Key::F11:
            send_vt_escape(23, modifiers);
            return;
        case App::Key::F12:
            send_vt_escape(24, modifiers);
            return;
        default:
            break;
    }

    auto ascii = [&]() -> char {
        if (event.control_down()) {

            switch (event.key()) {
                case App::Key::A:
                case App::Key::B:
                case App::Key::C:
                case App::Key::D:
                case App::Key::E:
                case App::Key::F:
                case App::Key::G:
                case App::Key::H:
                case App::Key::I:
                case App::Key::J:
                case App::Key::K:
                case App::Key::L:
                case App::Key::M:
                case App::Key::N:
                case App::Key::O:
                case App::Key::P:
                case App::Key::Q:
                case App::Key::R:
                case App::Key::S:
                case App::Key::T:
                case App::Key::U:
                case App::Key::V:
                case App::Key::W:
                case App::Key::X:
                case App::Key::Y:
                case App::Key::Z:
                    return (static_cast<int>(event.key()) - static_cast<int>(App::Key::A) + 'A') & 0x1F;
                case App::Key::Slash:
                    return '/' & 0x1F;
                case App::Key::Backslash:
                    return '\\' & 0x1F;
                case App::Key::RightBracket:
                    return '[' & 0x1F;
                case App::Key::LeftBracket:
                    return ']' & 0x1F;
                default:
                    return '\0';
            }
        }
        return '\0';
    }();

    if (ascii) {
        this->write(String { ascii });
    }
}

void PsuedoTerminal::handle_text_event(const App::TextEvent& event) {
    if (!event.text().empty()) {
        this->write(event.text());
    }
}

bool PsuedoTerminal::handle_mouse_event(const App::MouseEvent& event) {
    switch (m_mouse_tracking_mode) {
        case MouseTrackingMode::None:
        case MouseTrackingMode::Hilite:
            return false;
        case MouseTrackingMode::X10:
            if (!event.mouse_down()) {
                return false;
            }
            break;
        case MouseTrackingMode::X11:
            if (!event.mouse_down() && !event.mouse_up() && !event.mouse_scroll()) {
                return false;
            }
            break;
        case MouseTrackingMode::Cell:
            if (event.mouse_move() && !event.buttons_down()) {
                return false;
            }
            break;
        case MouseTrackingMode::All:
            break;
    }

    switch (m_mouse_reporting_mode) {
        case MouseReportingMode::None:
        case MouseReportingMode::X10:
        case MouseReportingMode::X11:
        case MouseReportingMode::Hilite:
        case MouseReportingMode::UTF8:
        case MouseReportingMode::URXVT:
            break;
        case MouseReportingMode::SGR: {
            int cb = event.z() < 0 ? 64 : event.z() > 0 ? 65 : 0;
            if (event.mouse_move()) {
                // FIXME: what to report if no button is held?
                cb = event.buttons_down() & App::MouseButton::Left ? 32 : 34;
            } else if (event.mouse_down() || event.mouse_up()) {
                cb = event.button() == App::MouseButton::Left ? 0 : 2;
            }
            write(String::format("\033[<%d;%d;%d%c", cb, event.x() + 1, event.y() + 1, event.mouse_up() ? 'm' : 'M'));
            break;
        }
    }

    return true;
}

void PsuedoTerminal::reset_mouse_tracking_mode(MouseTrackingMode tracking_mode) {
    if (m_mouse_tracking_mode == tracking_mode) {
        m_mouse_tracking_mode = MouseTrackingMode::None;
        reset_mouse_reporting_mode();
    }
}

void PsuedoTerminal::reset_mouse_reporting_mode() {
    switch (m_mouse_tracking_mode) {
        case MouseTrackingMode::None:
            m_mouse_reporting_mode = MouseReportingMode::None;
            break;
        case MouseTrackingMode::X10:
            m_mouse_reporting_mode = MouseReportingMode::X10;
            break;
        case MouseTrackingMode::X11:
        case MouseTrackingMode::Cell:
        case MouseTrackingMode::All:
            m_mouse_reporting_mode = MouseReportingMode::X11;
            break;
        case MouseTrackingMode::Hilite:
            m_mouse_reporting_mode = MouseReportingMode::Hilite;
            break;
    }
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
}
