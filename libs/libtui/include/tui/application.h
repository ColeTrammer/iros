#pragma once

#include <eventloop/event_loop.h>
#include <graphics/rect_set.h>
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
    virtual void on_key_event(const App::KeyEvent& event) override;
    virtual void on_text_event(const App::TextEvent& event) override;
    virtual void on_mouse_event(const App::MouseEvent& event) override;

    TInput::IOTerminal& io_terminal() { return *m_io_terminal; }

    App::EventLoop& event_loop() { return m_loop; }

    void set_focused_panel(Panel* panel);
    SharedPtr<Panel> focused_panel() const { return m_focused_panel.lock(); }

    const RectSet& dirty_rects() const { return m_dirty_rects; }

private:
    Application(UniquePtr<TInput::IOTerminal> io_terminal);

    App::MouseEvent translate_mouse_event(const Panel& panel, const App::MouseEvent& event) const;
    Panel* hit_test(const Panel& panel, const Point& point) const;

    void detect_cursor_position();
    void schedule_render();

    App::EventLoop m_loop;
    TInput::TerminalInputParser m_parser;
    UniquePtr<TInput::IOTerminal> m_io_terminal;
    WeakPtr<Panel> m_focused_panel;
    Maybe<Point> m_cursor_position;
    RectSet m_dirty_rects;
    bool m_use_alternate_screen_buffer { false };
    bool m_use_mouse { false };
    bool m_render_scheduled { false };
};
}
