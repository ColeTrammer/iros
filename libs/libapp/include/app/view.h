#pragma once

#include <app/base/view.h>
#include <app/base/view_bridge.h>
#include <app/forward.h>
#include <app/scroll_component.h>
#include <app/widget.h>

namespace App {
class View
    : public Widget
    , public Base::ViewBridge
    , public ScrollComponent {
    APP_WIDGET_BASE(Base::View, Widget, View, self, self, self)

    APP_BASE_VIEW_INTERFACE_FORWARD(base())

public:
    virtual void did_attach() override;

protected:
    View();

private:
    // ^Base::ViewBridge
    virtual void invalidate_all() override;
};
}
