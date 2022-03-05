#pragma once

#include <app/base/widget.h>
#include <app/forward.h>
#include <eventloop/forward.h>
#include <graphics/color.h>
#include <graphics/font.h>
#include <graphics/forward.h>
#include <graphics/palette.h>
#include <graphics/rect.h>

namespace App {
class Widget : public Base::Widget {
    APP_OBJECT(Widget)

public:
    virtual void initialize() override;
    virtual ~Widget() override;

    SharedPtr<Font> font() const { return m_font; }
    void set_font(SharedPtr<Font> font) { m_font = move(font); }

    void set_context_menu(SharedPtr<ContextMenu> menu);

    Widget* parent_widget() { return static_cast<Widget*>(Base::Widget::parent_widget()); }
    Window* parent_window();

    Color background_color() const { return m_palette->color(Palette::Background); }
    Color text_color() const { return m_palette->color(Palette::Text); }
    Color outline_color() const { return m_palette->color(Palette::Outline); }

    SharedPtr<Palette> palette() const { return m_palette; }

    Renderer get_renderer();

protected:
    Widget();

private:
    virtual bool is_widget() const final { return true; }

    SharedPtr<Font> m_font { Font::default_font() };
    SharedPtr<Palette> m_palette;
    SharedPtr<ContextMenu> m_context_menu;
};

}
