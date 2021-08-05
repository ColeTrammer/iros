#pragma once

#include <eventloop/selectable.h>
#include <ext/file.h>
#include <graphics/point.h>
#include <graphics/rect.h>
#include <liim/forward.h>
#include <liim/function.h>
#include <liim/pointers.h>
#include <termios.h>
#include <tinput/forward.h>
#include <tinput/terminal_text_style.h>

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

    void detect_cursor_position();

    void scroll_up(int times);
    void reset_cursor();
    void move_cursor_to(const Point& position);
    void put_style(const TerminalTextStyle& style);
    void put_glyph(const Point& position, const TerminalGlyph& glyph, const TerminalTextStyle& style = {});

    const Rect& terminal_rect() const { return m_terminal_rect; }

    const Point& initial_cursor_position() const { return m_initial_cursor_position; }

    Function<void(Span<const uint8_t>)> on_recieved_input;
    Function<void(const Rect&)> on_resize;

    IOTerminal(const termios& saved_termios, const termios& current_termios, const Rect& m_terminal_rect, UniquePtr<Ext::File> file);

private:
    enum class ColorRole { Foreground, Background };
    String color_string(Maybe<Color> color, ColorRole role) const;

    UniquePtr<Ext::File> m_file;
    SharedPtr<App::FdWrapper> m_selectable;
    TerminalTextStyle m_current_text_style;
    Point m_cursor_position;
    Point m_initial_cursor_position;
    Rect m_terminal_rect;
    termios m_saved_termios;
    termios m_current_termios;
    bool m_did_set_termios { false };
    bool m_show_cursor { true };
    bool m_use_alternate_screen_buffer { false };
    bool m_use_mouse { false };
    bool m_bracketed_paste { false };
};
}
