#pragma once

#include <app/base/terminal_widget.h>
#include <app/widget.h>
#include <eventloop/selectable.h>
#include <liim/pointers.h>

namespace App {
class TerminalWidget final
    : public Widget
    , public Base::TerminalWidget {
    APP_OBJECT(TerminalWidget)

    APP_EMITS(Widget, TerminalHangupEvent)

public:
    explicit TerminalWidget(double opacity);
    virtual void initialize() override;

    // ^App::Widget
    virtual void render() override;

    // ^Base::TerminalWidget
    virtual App::Object& this_widget() override { return *this; }
    virtual void invalidate_all_contents() override { invalidate(); }
    virtual Point cell_position_of_mouse_coordinates(int mouse_x, int mouse_y) const override;
    virtual Rect available_cells() const override;

private:
    uint8_t m_background_alpha { 255 };
};
}
