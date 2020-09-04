#pragma once

#include <app/object.h>
#include <graphics/font.h>
#include <graphics/rect.h>

namespace App {

class ContextMenu;
class Layout;
class KeyEvent;
class MouseEvent;
class Window;

struct Size {
    enum { Auto = -1 };

    int width { 0 };
    int height { 0 };
};

class Widget : public Object {
    APP_OBJECT(Widget)

public:
    virtual ~Widget() override;

    virtual void render();
    virtual void on_mouse_event(MouseEvent&);
    virtual void on_key_event(KeyEvent&) {}
    virtual void on_resize();
    virtual void on_focused() {}
    virtual void on_leave() {}

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

    const Font& font() const { return m_font; }
    void set_font(Font font) { m_font = move(font); }

    const Size& preferred_size() const { return m_preferred_size; }
    void set_preferred_size(const Size& size);

    void invalidate() { invalidate(rect()); }
    void invalidate(const Rect& rect);

    void set_context_menu(SharedPtr<ContextMenu> menu);

protected:
    Widget();

private:
    virtual bool is_widget() const final { return true; }

    Rect m_rect;
    Font m_font { Font::default_font() };
    Size m_preferred_size { Size::Auto, Size::Auto };
    UniquePtr<Layout> m_layout;
    SharedPtr<ContextMenu> m_context_menu;
    bool m_hidden { false };
};

}
