#pragma once

#include <eventloop/timer.h>
#include <tui/panel.h>

class TerminalDisplay;

class TerminalStatusBar final : public TUI::Panel {
    APP_OBJECT(TerminalStatusBar)

public:
    static TerminalStatusBar& the();

    TerminalStatusBar();
    virtual ~TerminalStatusBar() override;

    void set_active_display(TerminalDisplay* display);
    void set_status_message(String message);

    virtual void render() override;

private:
    WeakPtr<TerminalDisplay> m_active_display;
    Maybe<String> m_status_message;
    SharedPtr<App::Timer> m_status_message_timer;
};
