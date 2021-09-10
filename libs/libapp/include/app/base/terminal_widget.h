#pragma once

#include <eventloop/forward.h>
#include <eventloop/selectable.h>
#include <graphics/rect.h>
#include <liim/pointers.h>
#include <liim/string.h>
#include <terminal/pseudo_terminal.h>
#include <terminal/tty.h>

APP_EVENT(App, TerminalHangupEvent, Event, (), (), ())

namespace App::Base {
class TerminalWidget {
public:
    TerminalWidget();
    void initialize();
    virtual ~TerminalWidget();

    virtual App::Object& this_widget() = 0;
    virtual void invalidate_all_contents() = 0;

    virtual Point cell_position_of_mouse_coordinates(int mouse_x, int mouse_y) const = 0;
    virtual Rect available_cells() const = 0;

    void clear_selection();
    void copy_selection();
    void paste_text();

protected:
    const Terminal::TTY& tty() const { return m_tty; }
    bool in_selection(int row, int col) const;

private:
    String selection_text() const;

    Terminal::PsuedoTerminal m_pseudo_terminal;
    Terminal::TTY m_tty;
    SharedPtr<FdWrapper> m_pseudo_terminal_wrapper;
    int m_selection_start_row { -1 };
    int m_selection_start_col { -1 };
    int m_selection_end_row { -1 };
    int m_selection_end_col { -1 };
    bool m_in_selection { false };
};
}
