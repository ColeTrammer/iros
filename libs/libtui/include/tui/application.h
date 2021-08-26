#pragma once

#include <app/base/application.h>
#include <eventloop/event_loop.h>
#include <graphics/rect_set.h>
#include <liim/pointers.h>
#include <tinput/forward.h>
#include <tinput/terminal_input_parser.h>
#include <tui/panel.h>
#include <tui/root_window.h>

namespace TUI {
class Application : public App::Base::Application {
    APP_OBJECT(Application)

public:
    static SharedPtr<Application> try_create();
    static Application& the();

    virtual void initialize() override;
    virtual ~Application() override;

    virtual App::Margins default_margins() const override;
    virtual int default_spacing() const override;

    void set_use_alternate_screen_buffer(bool b) { m_use_alternate_screen_buffer = b; }
    void set_use_mouse(bool b) { m_use_mouse = b; }

    TInput::IOTerminal& io_terminal() { return *m_io_terminal; }

    RootWindow& root_window() { return *m_root_window; }

private:
    explicit Application(UniquePtr<TInput::IOTerminal> io_terminal);

    virtual void before_enter() override;

    void detect_cursor_position();
    void schedule_render();

    TInput::TerminalInputParser m_parser;
    UniquePtr<TInput::IOTerminal> m_io_terminal;
    SharedPtr<RootWindow> m_root_window;
    bool m_use_alternate_screen_buffer { false };
    bool m_use_mouse { false };
    bool m_render_scheduled { false };
};
}
