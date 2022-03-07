#pragma once

#include <app/base/view.h>
#include <app/base/view_bridge.h>
#include <tui/panel.h>

namespace TUI {
class View
    : public Panel
    , public App::Base::ViewBridge {
    APP_WIDGET_BASE(App::Base::View, Panel, View, self, self, nullptr)

    APP_BASE_VIEW_INTERFACE_FORWARD(base())

protected:
    View() {}

private:
    // ^Base::View
    virtual void invalidate_all() override;
};
}
