#pragma once

#include <app/scroll_component.h>
#include <eventloop/component.h>
#include <graphics/forward.h>
#include <graphics/point.h>
#include <graphics/rect.h>
#include <gui/forward.h>

namespace GUI {
class ScrollComponent : public App::ScrollComponentBridge {
    APP_SCROLL_COMPONENT_INTERFACE_FORWARD(base())

public:
    static constexpr int s_scrollbar_width = 16;

    explicit ScrollComponent(Widget& widget);
    virtual ~ScrollComponent() override;

    void attach_to_base(App::ScrollComponent& base);

    virtual int scrollbar_width() const override { return s_scrollbar_width; }

    Renderer get_renderer();
    void draw_scrollbars();

private:
    void did_attach();

    App::ScrollComponent& base() { return *m_base; }
    const App::ScrollComponent& base() const { return *m_base; }

    Widget& widget() { return m_widget; }
    const Widget& widget() const { return m_widget; }

    Widget& m_widget;
    App::ScrollComponent* m_base { nullptr };
};
}
