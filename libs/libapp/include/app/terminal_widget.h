#pragma once

#include <app/base/terminal_widget.h>
#include <app/base/terminal_widget_bridge.h>
#include <app/widget.h>
#include <eventloop/selectable.h>
#include <liim/pointers.h>

namespace App {
class TerminalWidget final
    : public Widget
    , public Base::TerminalWidgetBridge {
    APP_WIDGET_BASE(Base::TerminalWidget, Widget, TerminalWidget, self, self)

    APP_BASE_TERMINAL_WIDGET_INTERFACE_FORWARD(base())

public:
    explicit TerminalWidget(double opacity);
    virtual void did_attach() override;

    // ^App::Widget
    virtual void render() override;

    // ^Base::TerminalWidgetBridge
    virtual void invalidate_all_contents() override { invalidate(); }
    virtual Point cell_position_of_mouse_coordinates(int mouse_x, int mouse_y) const override;
    virtual Rect available_cells() const override;

private:
    uint8_t m_background_alpha { 255 };
};
}
