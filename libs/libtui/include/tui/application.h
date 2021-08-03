#pragma once

#include <eventloop/event_loop.h>
#include <graphics/rect.h>
#include <liim/pointers.h>
#include <tinput/forward.h>
#include <tinput/terminal_input_parser.h>
#include <tui/panel.h>

namespace TUI {
class Application : public Panel {
    APP_OBJECT(Application)

public:
    static SharedPtr<Application> try_create();
    static Application& the();

    ~Application();

    void set_use_alternate_screen_buffer(bool b) { m_use_alternate_screen_buffer = b; }
    void set_use_mouse(bool b) { m_use_mouse = b; }

    void enter();

    void invalidate();
    void invalidate(const Rect& rect);

    virtual void render() override;

    TInput::IOTerminal& io_terminal() { return *m_io_terminal; }

    App::EventLoop& event_loop() { return m_loop; }

private:
    Application(UniquePtr<TInput::IOTerminal> io_terminal);

    void detect_cursor_position();
    void schedule_render();

    App::EventLoop m_loop;
    TInput::TerminalInputParser m_parser;
    UniquePtr<TInput::IOTerminal> m_io_terminal;
    bool m_use_alternate_screen_buffer { false };
    bool m_use_mouse { false };
    bool m_render_scheduled { false };
};
}
