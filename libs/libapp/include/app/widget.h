#pragma once

#include <app/object.h>
#include <graphics/rect.h>

namespace App {

class Layout;
class KeyEvent;
class MouseEvent;
class Window;

class Widget : public Object {
    APP_OBJECT(Widget)

public:
    virtual ~Widget() override;

    virtual void render();
    virtual void on_mouse_event(MouseEvent&) {}
    virtual void on_key_event(KeyEvent&) {}
    virtual void on_resize();

    void set_rect(const Rect& rect);
    const Rect& rect() const { return m_rect; }

    Window* window();

    Layout* layout() { return m_layout.get(); }
    const Layout* layout() const { return m_layout.get(); }

    template<typename LayoutClass, typename... Args>
    LayoutClass& set_layout(Args&&... args) {
        auto layout = make_unique<LayoutClass>(*this, forward<Args>(args)...);
        m_layout = move(layout);
        return static_cast<LayoutClass&>(*m_layout);
    }

    bool hidden() const { return m_hidden; }
    void set_hidden(bool b) { m_hidden = b; }

protected:
    Widget();

private:
    virtual bool is_widget() const final { return true; }

    Rect m_rect;
    UniquePtr<Layout> m_layout;
    bool m_hidden { false };
};

}
