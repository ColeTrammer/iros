#pragma once

#include <eventloop/selectable.h>
#include <ext/file.h>
#include <graphics/rect.h>
#include <liim/forward.h>
#include <liim/function.h>
#include <liim/pointers.h>
#include <termios.h>

namespace TInput {
class IOTerminal {
public:
    static UniquePtr<IOTerminal> create(FILE* file);

    ~IOTerminal();

    void set_use_alternate_screen_buffer(bool b);
    void set_use_mouse(bool b);
    void set_bracketed_paste(bool b);
    void set_show_cursor(bool b);

    void flush();

    const Rect& terminal_rect() const { return m_terminal_rect; }

    Function<void(Span<const uint8_t>)> on_recieved_input;
    Function<void(const Rect&)> on_resize;

    IOTerminal(termios& saved_termios, termios& current_termios, const Rect& m_terminal_rect, UniquePtr<Ext::File> file);

private:
    UniquePtr<Ext::File> m_file;
    SharedPtr<App::FdWrapper> m_selectable;
    Rect m_terminal_rect;
    termios m_saved_termios;
    termios m_current_termios;
    bool m_did_set_termios { false };
};
}
