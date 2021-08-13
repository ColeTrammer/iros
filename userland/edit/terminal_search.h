#pragma once

#include <edit/forward.h>
#include <tui/frame.h>

class TerminalDisplay;

class TerminalSearch final : public TUI::Frame {
    APP_OBJECT(TerminalSearch)

public:
    TerminalSearch(TerminalDisplay& hots_display, String initial_text);
    virtual void initialize() override;
    virtual ~TerminalSearch() override;

private:
    TerminalDisplay& m_host_display;
    String m_initial_text;
};
