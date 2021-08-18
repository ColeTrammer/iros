#pragma once

#include <liim/function.h>
#include <liim/string.h>
#include <tui/frame.h>

class TerminalDisplay;

class TerminalPrompt final : public TUI::Frame {
    APP_OBJECT(TerminalPrompt)

public:
    TerminalPrompt(TerminalDisplay& host_display, String prompt, String initial_value);
    virtual void initialize() override;
    virtual ~TerminalPrompt() override;

    Function<void(String)> on_submit;

private:
    TerminalDisplay& m_host_display;
    String m_prompt;
    String m_initial_value;
    SharedPtr<TerminalDisplay> m_display;
};