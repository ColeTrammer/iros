#pragma once

#include <app/base/scroll_component.h>
#include <app/forward.h>
#include <eventloop/component.h>
#include <graphics/forward.h>
#include <graphics/point.h>

namespace App {
class ScrollComponent : public Base::ScrollComponentBridge {
    APP_BASE_SCROLL_COMPONENT_INTERFACE_FORWARD(base())

public:
    static constexpr int s_scrollbar_width = 16;

    explicit ScrollComponent(Widget& widget);
    virtual ~ScrollComponent() override;

    void attach_to_base(Base::ScrollComponent& base);

    virtual int scrollbar_width() const override { return s_scrollbar_width; }

    Renderer get_renderer();
    void draw_scrollbars();

private:
    void did_attach();

    Base::ScrollComponent& base() { return *m_base; }
    const Base::ScrollComponent& base() const { return *m_base; }

    Widget& widget() { return m_widget; }
    const Widget& widget() const { return m_widget; }

    Widget& m_widget;
    Base::ScrollComponent* m_base { nullptr };
};
}
