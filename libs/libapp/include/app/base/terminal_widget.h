#pragma once

#include <app/base/terminal_widget_bridge.h>
#include <app/base/terminal_widget_interface.h>
#include <app/base/widget.h>
#include <app/forward.h>
#include <eventloop/forward.h>
#include <graphics/rect.h>
#include <liim/pointers.h>
#include <liim/string.h>
#include <terminal/pseudo_terminal.h>
#include <terminal/tty.h>

APP_EVENT(App, TerminalHangupEvent, Event, (), (), ())

namespace App::Base {
class TerminalWidget : public Widget {
    APP_OBJECT(TerminalWidget)

    APP_EMITS(Widget, TerminalHangupEvent)

    APP_BASE_TERMINAL_WIDGET_BRIDGE_INTERFACE_FORWARD(bridge())

public:
    virtual void initialize() override;
    virtual ~TerminalWidget() override;

    // os_2 reflect begin
    void clear_selection();
    void copy_selection();
    void paste_text();

    const Terminal::TTY& tty() const { return m_tty; }
    bool in_selection(int row, int col) const;
    // os_2 reflect end

    const TerminalWidgetBridge& bridge() const { return *m_bridge; }
    TerminalWidgetBridge& bridge() { return *m_bridge; }

protected:
    TerminalWidget(SharedPtr<WidgetBridge> widget_bridge, SharedPtr<TerminalWidgetBridge> terminal_bridge);

private:
    String selection_text() const;

    Terminal::PsuedoTerminal m_pseudo_terminal;
    Terminal::TTY m_tty;
    SharedPtr<TerminalWidgetBridge> m_bridge;
    SharedPtr<FdWrapper> m_pseudo_terminal_wrapper;
    int m_selection_start_row { -1 };
    int m_selection_start_col { -1 };
    int m_selection_end_row { -1 };
    int m_selection_end_col { -1 };
    bool m_in_selection { false };
};
}
