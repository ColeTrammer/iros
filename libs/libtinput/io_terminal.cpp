#include <eventloop/event_loop.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <tinput/io_terminal.h>
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

    tcsetattr(fd, TCSANOW, &raw_termios);

    setvbuf(file, nullptr, _IOFBF, BUFSIZ);

    winsize size;
    ioctl(fd, TIOCGWINSZ, &size);

    return make_unique<IOTerminal>(saved_termios, raw_termios, Rect { 0, 0, size.ws_col, size.ws_row }, make_unique<Ext::File>(file));
}

IOTerminal::IOTerminal(termios& saved_termios, termios& current_termios, const Rect& terminal_rect, UniquePtr<Ext::File> file)
    : m_file(move(file))
    , m_terminal_rect(terminal_rect)
    , m_saved_termios(saved_termios)
    , m_current_termios(current_termios)
    , m_did_set_termios(true) {
    m_selectable = App::FdWrapper::create(nullptr, file->fd());
    m_selectable->set_selected_events(App::NotifyWhen::Readable);
    m_selectable->on_readable = [this] {
        auto buffer = ByteBuffer { BUFSIZ };
        while (m_file->read(buffer)) {
            if (on_recieved_input) {
                on_recieved_input(buffer.span());
            }
            buffer.set_size(0);
        }
    };
    m_selectable->enable_notifications();

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
    fprintf(m_file->c_file(), "\033[?2004%c", b ? 'h' : 'l');
}

void IOTerminal::set_use_alternate_screen_buffer(bool b) {
    fprintf(m_file->c_file(), "\033[?1049%c", b ? 'h' : 'l');
}

void IOTerminal::set_use_mouse(bool b) {
    fprintf(m_file->c_file(), "\033[?1002%c\033[?1006%c", b ? 'h' : 'l', b ? 'h' : 'l');
}

void IOTerminal::set_show_cursor(bool b) {
    fprintf(m_file->c_file(), "\033[?25%c", b ? 'h' : 'l');
}

void IOTerminal::flush() {
    fflush(m_file->c_file());
}
}
