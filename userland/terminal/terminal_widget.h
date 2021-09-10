#pragma once

#include <app/widget.h>
#include <eventloop/selectable.h>
#include <liim/pointers.h>

#include "base_terminal_widget.h"

class TerminalWidget final
    : public App::Widget
    , public BaseTerminalWidget {
    APP_OBJECT(TerminalWidget)

public:
    explicit TerminalWidget(double opacity);
    virtual void initialize() override;

    // ^App::Widget
    virtual void render() override;

    // ^BaseTerminalWidget
    virtual App::Object& this_widget() override { return *this; }
    virtual void invalidate_all_contents() override { invalidate(); }
    virtual Point cell_position_of_mouse_coordinates(int mouse_x, int mouse_y) const override;
    virtual Rect available_cells() const override;

private:
    uint8_t m_background_alpha { 255 };
};
