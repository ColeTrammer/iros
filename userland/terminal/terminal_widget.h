#pragma once

#include <app/selectable.h>
#include <app/widget.h>
#include <liim/pointers.h>

#include "pseudo_terminal.h"
#include "tty.h"

class TerminalWidget final : public App::Widget {
    APP_OBJECT(TerminalWidget)

public:
    TerminalWidget();

    virtual void render() override;
    virtual void on_resize() override;
    virtual void on_key_event(App::KeyEvent& event);
    virtual void on_mouse_event(App::MouseEvent& event);

private:
    void clear_selection();
    bool in_selection(int row, int col) const;
    String selection_text() const;

    TTY m_tty;
    PsuedoTerminal m_pseudo_terminal;
    SharedPtr<App::FdWrapper> m_pseudo_terminal_wrapper;
    int m_selection_start_row { -1 };
    int m_selection_start_col { -1 };
    int m_selection_end_row { -1 };
    int m_selection_end_col { -1 };
    bool m_in_selection { false };
};
