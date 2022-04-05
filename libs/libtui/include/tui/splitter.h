#pragma once

#include <app/splitter.h>
#include <tui/panel.h>

namespace TUI {
class Splitter
    : public Panel
    , public App::SplitterBridge {
    APP_WIDGET_BASE(App::Splitter, Panel, Splitter, self, self)

    APP_SPLITTER_INTERFACE_FORWARD(base());

public:
    Splitter();
    virtual ~Splitter() override;

    // ^TUI::Panel
    virtual void render() override;

    // ^App::SplitterBridge
    virtual int gutter_width() const override { return 1; }
};
}
