#pragma once

#include <app/base/terminal_widget.h>
#include <tui/panel.h>

namespace TUI {
class TerminalPanel
    : public TUI::Panel
    , public App::Base::TerminalWidget {
    APP_OBJECT(TerminalPanel)

    APP_EMITS(TUI::Panel, App::TerminalHangupEvent)

public:
    TerminalPanel();
    virtual void initialize() override;

    // ^TUI::Panel
    virtual Maybe<Point> cursor_position() override;
    virtual void render() override;

    // ^App::Base::BaseTerminalWidget
    virtual Object& this_widget() override { return *this; }
    virtual void invalidate_all_contents() override { invalidate(); }
    virtual Rect available_cells() const override { return sized_rect(); }
    virtual Point cell_position_of_mouse_coordinates(int mouse_x, int mouse_y) const override { return { mouse_x, mouse_y }; }
};
}
