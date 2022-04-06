#pragma once

#include <app/forward.h>
#include <app/layout_constraint.h>
#include <app/widget_bridge.h>
#include <app/widget_interface.h>
#include <eventloop/key_bindings.h>
#include <eventloop/object.h>
#include <eventloop/widget_events.h>
#include <graphics/rect.h>

namespace App {
class Widget : public Object {
    APP_OBJECT(Widget)

    APP_EMITS(Object, ResizeEvent, FocusedEvent, UnfocusedEvent, LeaveEvent, EnterEvent, ShowEvent, HideEvent, KeyDownEvent, KeyUpEvent,
              TextEvent, MouseDownEvent, MouseMoveEvent, MouseUpEvent, MouseScrollEvent, ThemeChangeEvent)

    APP_WIDGET_BRIDGE_INTERFACE_FORWARD(bridge())

public:
    virtual void initialize() override;
    virtual ~Widget() override;

    // os_2 reflect begin
    void remove();
    App::Widget* parent_widget();
    App::Window* parent_window();

    void invalidate();
    void invalidate(const Rect& rect);

    bool focused();
    void make_focused();
    bool accepts_focus() const { return m_accepts_focus; }
    void set_accepts_focus(bool b) { m_accepts_focus = b; }

    App::KeyBindings& key_bindings() { return m_key_bindings; }
    const App::KeyBindings& key_bindings() const { return m_key_bindings; }
    void set_key_bindings(App::KeyBindings key_bindings) { m_key_bindings = move(key_bindings); }

    bool hidden() const { return m_hidden; }
    void set_hidden(bool b);

    App::Widget* focus_proxy() const { return m_focus_proxy; }
    void set_focus_proxy(App::Widget* widget) { m_focus_proxy = widget; }

    Rect sized_rect() const { return m_positioned_rect.positioned(0); }
    const Rect& positioned_rect() const { return m_positioned_rect; }
    void set_positioned_rect(const Rect& rect);

    void flush_layout();

    const App::LayoutConstraint& layout_constraint() const { return m_layout_constraint; }
    void set_layout_constraint(const App::LayoutConstraint& constraint);

    const App::LayoutConstraint& min_layout_constraint() const { return m_min_layout_constraint; }
    void set_min_layout_constraint(const App::LayoutConstraint& constraint);

    template<typename WidgetType, typename... Args>
    SharedPtr<WidgetType> create_widget_owned(Args&&... args) {
        return WidgetType::create_owned(this, forward<Args>(args)...);
    }

    template<typename WidgetType, typename... Args>
    WidgetType& create_widget(Args&&... args) {
        return *create_widget_owned<WidgetType>(forward<Args>(args)...);
    }

    template<typename LayoutEngineType, typename... Args>
    LayoutEngineType& set_layout_engine(Args&&... args) {
        auto engine = make_unique<LayoutEngineType>(*this, forward<Args>(args)...);
        auto& ret = *engine;
        do_set_layout_engine(move(engine));
        return ret;
    }

    App::LayoutEngine* layout_engine() { return m_layout_engine.get(); }
    const App::LayoutEngine* layout_engine() const { return m_layout_engine.get(); }
    // os_2 reflect end

    WidgetBridge& bridge() { return *m_bridge; }

    void render_including_children();

protected:
    explicit Widget(SharedPtr<WidgetBridge> bridge);

private:
    virtual bool is_base_widget() const final override { return true; }

    void do_set_layout_engine(UniquePtr<LayoutEngine> engine);
    void relayout();

    Rect m_positioned_rect;
    LayoutConstraint m_layout_constraint;
    LayoutConstraint m_min_layout_constraint;
    UniquePtr<LayoutEngine> m_layout_engine;
    SharedPtr<WidgetBridge> m_bridge;
    App::KeyBindings m_key_bindings;
    Widget* m_focus_proxy { nullptr };
    bool m_accepts_focus { false };
    bool m_hidden { false };
};
}
