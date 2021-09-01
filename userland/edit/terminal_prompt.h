#pragma once

#include <liim/function.h>
#include <liim/string.h>
#include <tui/frame.h>

class TerminalDisplay;

APP_EVENT(Edit, PromptResult, App::Event, (), ((Maybe<String>, result)), ())

class TerminalPrompt final : public TUI::Frame {
    APP_OBJECT(TerminalPrompt)

    APP_EMITS(TUI::Frame, Edit::PromptResult)

public:
    TerminalPrompt(TerminalDisplay& host_display, String prompt, String initial_value);
    virtual void initialize() override;
    virtual ~TerminalPrompt() override;

    Task<Maybe<String>> block_until_result();

private:
    TerminalDisplay& m_host_display;
    String m_prompt;
    String m_initial_value;
    SharedPtr<TerminalDisplay> m_display;
};
