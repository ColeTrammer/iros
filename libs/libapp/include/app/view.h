#pragma once

#include <app/base/view.h>
#include <app/forward.h>
#include <app/widget.h>

namespace App {
class View
    : public Widget
    , public Base::View {
    APP_OBJECT(View)

    APP_EMITS(Widget, ViewRootChanged)

public:
    virtual void initialize() override {
        Base::View::initialize();
        Widget::initialize();
    }

private:
    // ^Base::View
    virtual View& this_widget() override { return *this; }
    virtual void invalidate_all() override { invalidate(); }
};
}
