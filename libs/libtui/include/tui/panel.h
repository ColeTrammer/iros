#pragma once

#include <app/forward.h>
#include <app/widget.h>
#include <app/widget_bridge.h>
#include <eventloop/component.h>
#include <eventloop/forward.h>
#include <graphics/rect.h>
#include <liim/forward.h>
#include <tinput/forward.h>
#include <tui/forward.h>

namespace TUI {
class Panel
    : public App::Component
    , public App::WidgetBridge {
    APP_WIDGET_BASE(App::Widget, App::WidgetBridge, Panel, self)

    APP_OBJECT_FORWARD_API(base())

    APP_WIDGET_INTERFACE_FORWARD(base())

public:
    Panel();
    virtual void did_attach() override;
    virtual ~Panel() override;

    TInput::TerminalRenderer get_renderer();
};
}
