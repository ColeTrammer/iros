#pragma once

#include <app/forward.h>
#include <eventloop/forward.h>
#include <eventloop/object.h>
#include <graphics/color.h>
#include <graphics/font.h>
#include <graphics/forward.h>
#include <graphics/palette.h>
#include <graphics/rect.h>

namespace App {
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
    virtual void on_mouse_event(const MouseEvent&);
    virtual void on_key_event(const KeyEvent&) {}
    virtual void on_theme_change_event(const ThemeChangeEvent&);
    virtual void on_resize();
    virtual void on_focused() {}
    virtual void on_leave() {}

    virtual void on_mouse_down(const MouseEvent&);
    virtual void on_mouse_double(const MouseEvent&);
    virtual void on_mouse_triple(const MouseEvent&);
    virtual void on_mouse_up(const MouseEvent&);
    virtual void on_mouse_move(const MouseEvent&);
    virtual void on_mouse_scroll(const MouseEvent&);

    void set_positioned_rect(const Rect& rect);
    const Rect& positioned_rect() const { return m_positioned_rect; }

    Rect sized_rect() const { return m_positioned_rect.positioned(0); }

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
    void set_hidden(bool b);

    const Font& font() const { return m_font; }
    void set_font(Font font) { m_font = move(font); }

    const Size& preferred_size() const { return m_preferred_size; }
    void set_preferred_size(const Size& size);

    void invalidate() { invalidate(positioned_rect()); }
    void invalidate(const Rect& rect);

    void set_context_menu(SharedPtr<ContextMenu> menu);

    Color background_color() const { return m_palette->color(Palette::Background); }
    Color text_color() const { return m_palette->color(Palette::Text); }
    Color outline_color() const { return m_palette->color(Palette::Outline); }

    SharedPtr<Palette> palette() const { return m_palette; }

protected:
    Widget();

    Renderer get_renderer();

private:
    virtual bool is_widget() const final { return true; }

    Rect m_positioned_rect;
    Font m_font { Font::default_font() };
    SharedPtr<Palette> m_palette;
    Size m_preferred_size { Size::Auto, Size::Auto };
    UniquePtr<Layout> m_layout;
    SharedPtr<ContextMenu> m_context_menu;
    bool m_hidden { false };
};

}
