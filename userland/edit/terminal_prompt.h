#pragma once

#include <liim/function.h>
#include <liim/string.h>
#include <tui/frame.h>

class TerminalDisplay;

APP_EVENT(Edit, PromptResult, App::Event, (), ((Maybe<String>, result)), ())

class TerminalPrompt final : public TUI::Frame {
    APP_WIDGET_EMITS(TUI::Frame, TerminalPrompt, (Edit::PromptResult))

public:
    TerminalPrompt(TerminalDisplay& host_display, String prompt, String initial_value);
    virtual void did_attach() override;
    virtual ~TerminalPrompt() override;

    Task<Maybe<String>> block_until_result(App::Object& coroutine_owner);

private:
    TerminalDisplay& m_host_display;
    String m_prompt;
    String m_initial_value;
    SharedPtr<TerminalDisplay> m_display;
};
