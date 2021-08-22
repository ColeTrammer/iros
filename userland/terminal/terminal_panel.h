#pragma once

#include <tui/panel.h>

#include "base_terminal_widget.h"

class TerminalPanel
    : public TUI::Panel
    , public BaseTerminalWidget {
    APP_OBJECT(TerminalPanel)

public:
    TerminalPanel();
    virtual void initialize() override;

    // ^TUI::Panel
    virtual Maybe<Point> cursor_position() override;
    virtual void render() override;

    // ^BaseTerminalWidget
    virtual Object& this_widget() override { return *this; }
    virtual void invalidate_all_contents() override { invalidate(); }
    virtual Rect available_cells() const override { return sized_rect(); }
    virtual Point cell_position_of_mouse_coordinates(int mouse_x, int mouse_y) const override { return { mouse_x, mouse_y }; }
};
