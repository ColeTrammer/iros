#pragma once

#include <app/view.h>
#include <app/view_bridge.h>
#include <tui/panel.h>
#include <tui/scroll_component.h>
namespace TUI {
class View
    : public Panel
    , public App::ViewBridge
    , public ScrollComponent {
    APP_WIDGET_BASE(App::View, Panel, View, self, self, self)

    APP_VIEW_INTERFACE_FORWARD(base())

public:
    virtual void did_attach() override;

protected:
    View();

private:
    // ^View
    virtual void invalidate_all() override;
};
}
