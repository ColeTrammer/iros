#pragma once

#include <app/base/view.h>
#include <app/forward.h>
#include <app/widget.h>

namespace App {
class View
    : public Widget
    , public Base::View {
    APP_OBJECT(View)

    APP_EMITS(Widget, ViewRootChanged, ViewItemActivated)

protected:
    View() : Base::View(static_cast<Object&>(*this)) {}

private:
    // ^Base::View
    virtual void invalidate_all() override { invalidate(); }
};
}
