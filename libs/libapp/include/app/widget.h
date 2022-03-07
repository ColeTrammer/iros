#pragma once

#include <app/base/widget.h>
#include <app/base/widget_bridge.h>
#include <app/forward.h>
#include <eventloop/component.h>
#include <eventloop/forward.h>
#include <graphics/color.h>
#include <graphics/font.h>
#include <graphics/forward.h>
#include <graphics/palette.h>
#include <graphics/rect.h>

namespace App {
class Widget
    : public Component
    , public Base::WidgetBridge {
    APP_WIDGET_BASE(Base::Widget, Base::WidgetBridge, Widget, self)

    APP_OBJECT_FORWARD_API(base())

    APP_BASE_WIDGET_INTERFACE_FORWARD(base())

public:
    Widget();
    virtual void did_attach() override;
    virtual ~Widget() override;

    SharedPtr<Font> font() const { return m_font; }
    void set_font(SharedPtr<Font> font) { m_font = move(font); }

    void set_context_menu(SharedPtr<ContextMenu> menu);

    Window* typed_parent_window();

    Color background_color() const { return m_palette->color(Palette::Background); }
    Color text_color() const { return m_palette->color(Palette::Text); }
    Color outline_color() const { return m_palette->color(Palette::Outline); }

    SharedPtr<Palette> palette() const { return m_palette; }

    Renderer get_renderer();

private:
    virtual bool is_widget() const final { return true; }

    SharedPtr<Font> m_font { Font::default_font() };
    SharedPtr<Palette> m_palette;
    SharedPtr<ContextMenu> m_context_menu;
};
}
