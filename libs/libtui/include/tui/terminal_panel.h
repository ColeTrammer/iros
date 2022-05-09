#pragma once

#include <app/terminal_widget.h>
#include <app/terminal_widget_bridge.h>
#include <tui/panel.h>

namespace TUI {
class TerminalPanel
    : public TUI::Panel
    , public App::TerminalWidgetBridge {
    APP_WIDGET_BASE(App::TerminalWidget, TUI::Panel, TerminalPanel, self, self)

    APP_TERMINAL_WIDGET_INTERFACE_FORWARD(base())

public:
    TerminalPanel();

    // ^TUI::Panel
    virtual Option<Point> cursor_position() override;
    virtual void render() override;

    // ^App::TerminalWidgetBridge
    virtual void invalidate_all_contents() override;
    virtual Rect available_cells() const override;
    virtual Point cell_position_of_mouse_coordinates(int mouse_x, int mouse_y) const override;
};
}
