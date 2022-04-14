#pragma once

#include <app/terminal_widget_bridge_interface.h>
#include <graphics/forward.h>

namespace App {
class TerminalWidgetBridge {
public:
    virtual ~TerminalWidgetBridge() {}

    // iros reflect begin
    virtual Point cell_position_of_mouse_coordinates(int mouse_x, int mouse_y) const = 0;
    virtual Rect available_cells() const = 0;
    // iros reflect end

    virtual void invalidate_all_contents() = 0;
};
}
