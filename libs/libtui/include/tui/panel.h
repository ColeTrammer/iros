#pragma once

#include <eventloop/object.h>
#include <graphics/rect.h>
#include <liim/forward.h>
#include <tinput/forward.h>
#include <tui/forward.h>

namespace TUI {
class LayoutConstraint {
public:
    static constexpr int AutoSize = -1;

    constexpr LayoutConstraint() {}
    constexpr LayoutConstraint(int width, int height) : m_width(width), m_height(height) {}

    constexpr int width() const { return m_width; }
    constexpr int height() const { return m_height; }

private:
    int m_width { AutoSize };
    int m_height { AutoSize };
};

class Panel : public App::Object {
    APP_OBJECT(Panel)

public:
    Panel();
    virtual ~Panel();

    virtual bool steals_focus() { return false; }

    virtual Maybe<Point> cursor_position();
    virtual void render();

    virtual void on_resize();
    virtual void on_focused() {}
    virtual void on_unfocused() {}

    virtual void on_key_down(const App::KeyEvent& event);
    virtual void on_key_up(const App::KeyEvent&) {}

    virtual void on_mouse_down(const App::MouseEvent&);
    virtual void on_mouse_double(const App::MouseEvent&);
    virtual void on_mouse_triple(const App::MouseEvent&);
    virtual void on_mouse_up(const App::MouseEvent&);
    virtual void on_mouse_move(const App::MouseEvent&);
    virtual void on_mouse_scroll(const App::MouseEvent&);

    virtual void on_key_event(const App::KeyEvent& event);
    virtual void on_text_event(const App::TextEvent&) {}
    virtual void on_mouse_event(const App::MouseEvent& event);

    virtual void on_event(const App::Event& event) override;

    void remove();
    Panel* parent_panel();

    void make_focused();
    bool accepts_focus() const { return m_accepts_focus; }
    void set_accepts_focus(bool b) { m_accepts_focus = b; }

    Panel* focus_proxy() const { return m_focus_proxy; }
    void set_focus_proxy(Panel* panel) { m_focus_proxy = panel; }

    const Rect& positioned_rect() const { return m_positioned_rect; }
    Rect sized_rect() const { return { 0, 0, m_positioned_rect.width(), m_positioned_rect.height() }; }

    void set_positioned_rect(const Rect& rect);

    const LayoutConstraint& layout_constraint() const { return m_layout_constraint; }
    void set_layout_constraint(const LayoutConstraint& constraint) { m_layout_constraint = constraint; }

    template<typename LayoutEngineType, typename... Args>
    LayoutEngineType& set_layout_engine(Args&&... args) {
        auto engine = make_unique<LayoutEngineType>(*this, forward<Args>(args)...);
        auto& ret = *engine;
        do_set_layout_engine(move(engine));
        return ret;
    }

    void invalidate();
    void invalidate(const Rect& rect);

    void set_key_bindings(UniquePtr<App::KeyBindings> bindings);
    bool handle_as_key_shortcut(const App::KeyEvent& event);

protected:
    TInput::TerminalRenderer get_renderer();

private:
    virtual bool is_panel() const final { return true; }

    void do_set_layout_engine(UniquePtr<LayoutEngine> engine);

    Rect m_positioned_rect;
    LayoutConstraint m_layout_constraint;
    UniquePtr<LayoutEngine> m_layout_engine;
    UniquePtr<App::KeyBindings> m_key_bindings;
    Panel* m_focus_proxy { nullptr };
    bool m_accepts_focus { false };
};
}
