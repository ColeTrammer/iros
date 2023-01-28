#pragma once

#include <edit/forward.h>
#include <tui/frame.h>

class TerminalDisplay;

class TerminalSearch final : public TUI::Frame {
    APP_WIDGET(TUI::Frame, TerminalSearch)

public:
    TerminalSearch(TerminalDisplay& host_display, String initial_text);
    virtual void did_attach() override;
    virtual ~TerminalSearch() override;

private:
    TerminalDisplay& m_host_display;
    String m_initial_text;
};
