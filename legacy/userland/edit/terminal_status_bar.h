#pragma once

#include <eventloop/timer.h>
#include <tui/panel.h>

class TerminalStatusBar final : public TUI::Panel {
    APP_WIDGET(TUI::Panel, TerminalStatusBar)

public:
    static TerminalStatusBar& the();

    TerminalStatusBar();
    virtual void did_attach() override;
    virtual ~TerminalStatusBar() override;

    void set_active_display(Edit::Display* display);
    void set_status_message(String message);
    void display_did_update(Edit::Display& display);

    virtual void render() override;

    Edit::Display& active_display() { return *m_active_display.lock(); }

private:
    WeakPtr<Edit::Display> m_active_display;
    Option<String> m_status_message;
    SharedPtr<App::Timer> m_status_message_timer;
};
