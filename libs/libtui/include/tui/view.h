#pragma once

#include <app/base/view.h>
#include <tui/panel.h>

namespace TUI {
class View
    : public Panel
    , public App::Base::View {
    APP_OBJECT(View)

    APP_EMITS(Panel, App::ViewRootChanged, App::ViewItemActivated)

public:
    virtual void initialize() override {
        App::Base::View::initialize();
        Panel::initialize();
    }

private:
    // ^Base::View
    virtual View& this_widget() override { return *this; }
    virtual void invalidate_all() override { invalidate(); }
};
}
