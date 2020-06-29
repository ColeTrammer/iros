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
    TTY m_tty;
    PsuedoTerminal m_pseudo_terminal;
    UniquePtr<App::FdWrapper> m_pseudo_terminal_wrapper;
};
