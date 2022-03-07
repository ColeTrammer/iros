#pragma once

#include <app/base/scroll_component.h>
#include <app/forward.h>
#include <eventloop/component.h>
#include <graphics/forward.h>
#include <graphics/point.h>

namespace App {
class ScrollComponent : public Base::ScrollComponent {
public:
    static constexpr int scrollbar_width = 16;

    explicit ScrollComponent(Object& object);
    virtual ~ScrollComponent() override;

    Renderer get_renderer();
    void draw_scrollbars();

private:
    Widget& widget() { return typed_object<Widget>(); }
};
}
