#pragma once

#include <app/base/view.h>
#include <tui/panel.h>

namespace TUI {
class View
    : public Panel
    , public App::Base::View {
    APP_OBJECT(View)

    APP_EMITS(Panel, App::ViewRootChanged, App::ViewItemActivated)

protected:
    View() : App::Base::View(static_cast<Object&>(*this)) {}

private:
    // ^Base::View
    virtual void invalidate_all() override { invalidate(); }
};
}
