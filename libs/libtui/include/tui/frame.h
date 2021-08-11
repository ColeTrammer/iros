#pragma once

#include <tinput/terminal_renderer.h>
#include <tui/panel.h>

namespace TUI {
class Frame : public TUI::Panel {
    APP_OBJECT(Frame)

public:
    virtual void render() override;

    Rect positioned_inner_rect() const;
    Rect relative_inner_rect() const;
    Rect sized_inner_rect() const;

    void set_frame_color(Maybe<Color> c) {
        m_frame_color = c;
        invalidate();
    }
    void set_box_style(TInput::TerminalRenderer::BoxStyle box_style) {
        m_box_style = box_style;
        invalidate();
    }

protected:
    TInput::TerminalRenderer get_renderer_inside_frame();

private:
    Maybe<Color> m_frame_color;
    TInput::TerminalRenderer::BoxStyle m_box_style { TInput::TerminalRenderer::BoxStyle::Thick };
};
}
