#pragma once

#include <app/view.h>
#include <app/view_bridge.h>
#include <gui/scroll_component.h>
#include <gui/widget.h>

namespace GUI {
class View
    : public Widget
    , public App::ViewBridge
    , public ScrollComponent {
    APP_WIDGET_BASE(App::View, Widget, View, self, self, self)

    APP_VIEW_INTERFACE_FORWARD(base())

public:
    virtual void did_attach() override;

protected:
    View();

private:
    // ^Base::ViewBridge
    virtual void invalidate_all() override;
};
}
