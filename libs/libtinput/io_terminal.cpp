#include <eventloop/event_loop.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <tinput/io_terminal.h>
#include <tinput/terminal_glyph.h>
#include <unistd.h>

namespace TInput {
UniquePtr<IOTerminal> IOTerminal::create(FILE* file) {
    int fd = fileno(file);
    if (!isatty(fd)) {
        return nullptr;
    }

    termios saved_termios;
    tcgetattr(fd, &saved_termios);

    termios raw_termios = saved_termios;
    cfmakeraw(&raw_termios);

    raw_termios.c_cc[VMIN] = 0;
    raw_termios.c_cc[VTIME] = 0;

    tcsetattr(fd, TCSANOW, &raw_termios);

    setvbuf(file, nullptr, _IOFBF, BUFSIZ);

    winsize size;
    ioctl(fd, TIOCGWINSZ, &size);

    fputs("\033[m", file);

    return make_unique<IOTerminal>(saved_termios, raw_termios, Rect { 0, 0, size.ws_col, size.ws_row }, make_unique<Ext::File>(file));
}

IOTerminal::IOTerminal(const termios& saved_termios, const termios& current_termios, const Rect& terminal_rect, UniquePtr<Ext::File> file)
    : m_file(move(file))
    , m_terminal_rect(terminal_rect)
    , m_saved_termios(saved_termios)
    , m_current_termios(current_termios)
    , m_did_set_termios(true) {
    m_file->set_should_close_file(false);
    m_selectable = App::FdWrapper::create(nullptr, m_file->fd());
    m_selectable->set_selected_events(App::NotifyWhen::Readable);
    m_selectable->enable_notifications();
    m_selectable->on_readable = [this] {
        auto buffer = ByteBuffer {};
        while (m_file->read(buffer)) {
            if (on_recieved_input) {
                on_recieved_input(buffer.span());
            }
            buffer.set_size(0);
        }
    };

    App::EventLoop::register_signal_handler(SIGWINCH, [this] {
        winsize size;
        ioctl(m_file->fd(), TIOCGWINSZ, &size);

        m_terminal_rect = { 0, 0, size.ws_col, size.ws_row };
        if (on_resize) {
            on_resize(m_terminal_rect);
        }
    });
}

IOTerminal::~IOTerminal() {
    App::EventLoop::unregister_signal_handler(SIGWINCH);

    set_bracketed_paste(false);
    set_use_alternate_screen_buffer(false);
    set_use_mouse(false);
    set_show_cursor(true);
    flush();
    if (m_did_set_termios) {
        tcsetattr(m_file->fd(), TCSANOW, &m_saved_termios);
    }
}

void IOTerminal::set_bracketed_paste(bool b) {
    if (b == m_bracketed_paste) {
        return;
    }
    m_bracketed_paste = b;
    fprintf(m_file->c_file(), "\033[?2004%c", b ? 'h' : 'l');
}

void IOTerminal::set_use_alternate_screen_buffer(bool b) {
    if (b == m_use_alternate_screen_buffer) {
        return;
    }
    m_use_alternate_screen_buffer = b;
    fprintf(m_file->c_file(), "\033[?1049%c", b ? 'h' : 'l');
}

void IOTerminal::set_use_mouse(bool b) {
    if (b == m_use_mouse) {
        return;
    }
    m_use_mouse = b;
    fprintf(m_file->c_file(), "\033[?1002%c\033[?1006%c", b ? 'h' : 'l', b ? 'h' : 'l');
}

void IOTerminal::set_show_cursor(bool b) {
    if (b == m_show_cursor) {
        return;
    }
    m_show_cursor = b;
    fprintf(m_file->c_file(), "\033[?25%c", b ? 'h' : 'l');
}

void IOTerminal::scroll_up(int times) {
    fprintf(m_file->c_file(), "\033[%dS", times);
}

void IOTerminal::detect_cursor_position() {
    fprintf(m_file->c_file(), "\033[6n");
    flush();

    auto start = time(nullptr);
    auto end = start + 1;
    for (;;) {
        auto now = time(nullptr);
        if (now > end) {
            break;
        }

        pollfd fds[1] = { { .fd = m_file->fd(), .events = POLL_IN, .revents = 0 } };
        auto ret = poll(fds, 1, now - end);
        if (ret <= 0) {
            break;
        }

        auto buffer = ByteBuffer {};
        if (!m_file->read(buffer)) {
            break;
        }

        auto string = String { (const char*) buffer.data(), buffer.size() };
        for (size_t i = 0; i < string.size(); i++) {
            int row;
            int col;
            auto ret = sscanf(string.string() + i, "\033[%d;%dR", &row, &col);
            if (ret != 2) {
                continue;
            }

            m_initial_cursor_position = { col - 1, row - 1 };
            m_cursor_position = m_initial_cursor_position;

            if (on_recieved_input) {
                auto before_sequence = string.view().first(i);
                auto after_sequence = string.view().substring(*string.view().substring(i).index_of('R') + 1);

                on_recieved_input({ (const uint8_t*) before_sequence.data(), before_sequence.size() });
                on_recieved_input({ (const uint8_t*) after_sequence.data(), after_sequence.size() });
            }
            return;
        }
    }

    // Since the cursor's location wasn't found, force it the top left of the screen.
    reset_cursor();
}

void IOTerminal::reset_cursor() {
    fprintf(m_file->c_file(), "\033[1;1H");
    m_cursor_position = {};
}

void IOTerminal::move_cursor_to(const Point& position) {
    if (m_cursor_position == position) {
        return;
    }

    fprintf(m_file->c_file(), "\033[%d;%dH", position.y() + 1, position.x() + 1);
    m_cursor_position = position;
}

String IOTerminal::color_string(Maybe<Color> color, ColorRole role) const {
    auto offset = role == ColorRole::Background ? 10 : 0;
    if (!color) {
        return String::format("%d", 39 + offset);
    }

    auto vga_color = color->to_vga_color();
    if (!vga_color) {
        // FIXME: separate the fields with ':' once its properly supported in the terminal.
        return String::format("%d;2;%d;%d;%d", 38 + offset, color->r(), color->b(), color->g());
    }

    auto number = [&] {
        switch (*vga_color) {
            case VGA_COLOR_BLACK:
                return 30;
            case VGA_COLOR_RED:
                return 31;
            case VGA_COLOR_GREEN:
                return 32;
            case VGA_COLOR_BROWN:
                return 33;
            case VGA_COLOR_BLUE:
                return 34;
            case VGA_COLOR_MAGENTA:
                return 35;
            case VGA_COLOR_CYAN:
                return 36;
            case VGA_COLOR_LIGHT_GREY:
                return 37;
            case VGA_COLOR_DARK_GREY:
                return 90;
            case VGA_COLOR_LIGHT_RED:
                return 91;
            case VGA_COLOR_LIGHT_GREEN:
                return 92;
            case VGA_COLOR_YELLOW:
                return 93;
            case VGA_COLOR_LIGHT_BLUE:
                return 94;
            case VGA_COLOR_LIGHT_MAGENTA:
                return 95;
            case VGA_COLOR_LIGHT_CYAN:
                return 96;
            case VGA_COLOR_WHITE:
                return 97;
            default:
                return 39;
        }
    }();
    return String::format("%d", number + offset);
}

void IOTerminal::put_style(const TerminalTextStyle& style) {
    if (m_current_text_style == style) {
        return;
    }

    auto string = "\033[0"s;
    if (style.bold) {
        string += ";1";
    }
    string += String::format(";%s", color_string(style.foreground, ColorRole::Foreground).string());
    string += String::format(";%s", color_string(style.background, ColorRole::Background).string());
    if (style.invert) {
        string += ";7";
    }
    string += "m";

    fputs(string.string(), m_file->c_file());
    m_current_text_style = style;
}

void IOTerminal::put_glyph(const Point& position, const TerminalGlyph& glyph, const TerminalTextStyle& style) {
    put_style(style);

    move_cursor_to(position);
    fwrite(glyph.text().string(), 1, glyph.text().size(), m_file->c_file());

    m_cursor_position.set_x(m_cursor_position.x() + glyph.width());
    if (m_cursor_position.x() > m_terminal_rect.width()) {
        m_cursor_position.set_x(0);
        m_cursor_position.set_y(min(m_cursor_position.y() + 1, m_terminal_rect.height() - 1));
    }
}

void IOTerminal::flush() {
    fflush(m_file->c_file());
}
}
