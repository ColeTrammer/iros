#pragma once

#include <app/base/widget.h>
#include <app/base/widget_bridge.h>
#include <app/forward.h>
#include <eventloop/component.h>
#include <eventloop/forward.h>
#include <graphics/rect.h>
#include <liim/forward.h>
#include <tinput/forward.h>
#include <tui/forward.h>

namespace TUI {
class Panel
    : public App::Component
    , public App::Base::WidgetBridge {
    APP_WIDGET_BASE(App::Base::Widget, App::Base::WidgetBridge, Panel, self)

    APP_OBJECT_FORWARD_API(base())

    APP_BASE_WIDGET_INTERFACE_FORWARD(base())

public:
    Panel();
    virtual void did_attach() override;
    virtual ~Panel() override;

protected:
    TInput::TerminalRenderer get_renderer();
};
}
