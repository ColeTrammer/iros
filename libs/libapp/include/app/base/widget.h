#pragma once

#include <app/forward.h>
#include <app/layout_constraint.h>
#include <eventloop/object.h>
#include <graphics/rect.h>

namespace App::Base {
class Widget : public Object {
    APP_OBJECT(Widget)

public:
    Widget();
    virtual void initialize() override;
    virtual ~Widget() override;

    virtual bool steals_focus() { return false; }

    virtual Maybe<Point> cursor_position();
    virtual void render();

    void remove();
    Widget* parent_widget();
    Window* parent_window();

    void invalidate();
    void invalidate(const Rect& rect);

    void make_focused();
    bool accepts_focus() const { return m_accepts_focus; }
    void set_accepts_focus(bool b) { m_accepts_focus = b; }

    bool hidden() const { return m_hidden; }
    void set_hidden(bool b);

    Widget* focus_proxy() const { return m_focus_proxy; }
    void set_focus_proxy(Widget* widget) { m_focus_proxy = widget; }

    Rect sized_rect() const { return m_positioned_rect.positioned(0); }
    const Rect& positioned_rect() const { return m_positioned_rect; }
    void set_positioned_rect(const Rect& rect);

    const LayoutConstraint& layout_constraint() const { return m_layout_constraint; }
    void set_layout_constraint(const LayoutConstraint& constraint);

    template<typename LayoutEngineType, typename... Args>
    LayoutEngineType& set_layout_engine(Args&&... args) {
        auto engine = make_unique<LayoutEngineType>(*this, forward<Args>(args)...);
        auto& ret = *engine;
        do_set_layout_engine(move(engine));
        return ret;
    }
    LayoutEngine* layout_engine() { return m_layout_engine.get(); }
    const LayoutEngine* layout_engine() const { return m_layout_engine.get(); }

private:
    virtual bool is_base_widget() const final override { return true; }

    void do_set_layout_engine(UniquePtr<LayoutEngine> engine);
    void relayout();

    Rect m_positioned_rect;
    LayoutConstraint m_layout_constraint;
    UniquePtr<LayoutEngine> m_layout_engine;
    Widget* m_focus_proxy { nullptr };
    bool m_accepts_focus { false };
    bool m_hidden { false };
};
}
