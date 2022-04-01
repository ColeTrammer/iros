#pragma once

#include <app/terminal_widget.h>
#include <app/terminal_widget_bridge.h>
#include <eventloop/selectable.h>
#include <gui/widget.h>
#include <liim/pointers.h>

namespace GUI {
class TerminalWidget
    : public Widget
    , public App::TerminalWidgetBridge {
    APP_WIDGET_BASE(App::TerminalWidget, Widget, TerminalWidget, self, self)

    APP_TERMINAL_WIDGET_INTERFACE_FORWARD(base())

public:
    explicit TerminalWidget(double opacity);
    virtual void did_attach() override;

    // ^GUI::Widget
    virtual void render() override;

    // ^App::TerminalWidgetBridge
    virtual void invalidate_all_contents() override { invalidate(); }
    virtual Point cell_position_of_mouse_coordinates(int mouse_x, int mouse_y) const override;
    virtual Rect available_cells() const override;

private:
    uint8_t m_background_alpha { 255 };
};
}
