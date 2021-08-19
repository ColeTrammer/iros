#pragma once

#include <app/widget.h>
#include <eventloop/selectable.h>
#include <liim/pointers.h>

#include "pseudo_terminal.h"
#include "tty.h"

class TerminalWidget final : public App::Widget {
    APP_OBJECT(TerminalWidget)

public:
    TerminalWidget(double opacity);
    virtual void initialize() override;

    virtual void render() override;

private:
    void clear_selection();
    bool in_selection(int row, int col) const;
    String selection_text() const;

    bool handle_mouse_event(const App::MouseEvent& event);

    void copy_selection();
    void paste_text();

    PsuedoTerminal m_pseudo_terminal;
    TTY m_tty;
    SharedPtr<App::FdWrapper> m_pseudo_terminal_wrapper;
    int m_selection_start_row { -1 };
    int m_selection_start_col { -1 };
    int m_selection_end_row { -1 };
    int m_selection_end_col { -1 };
    uint8_t m_background_alpha { 255 };
    bool m_in_selection { false };
};
