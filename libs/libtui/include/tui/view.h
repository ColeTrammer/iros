#pragma once

#include <app/base/view.h>
#include <app/base/view_bridge.h>
#include <tui/panel.h>
#include <tui/scroll_component.h>
namespace TUI {
class View
    : public Panel
    , public App::Base::ViewBridge
    , public ScrollComponent {
    APP_WIDGET_BASE(App::Base::View, Panel, View, self, self, self)

    APP_BASE_VIEW_INTERFACE_FORWARD(base())

public:
    virtual void did_attach() override;

protected:
    View();

private:
    // ^Base::View
    virtual void invalidate_all() override;
};
}
