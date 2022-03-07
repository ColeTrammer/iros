#pragma once

#include <tinput/terminal_renderer.h>
#include <tui/panel.h>

namespace TUI {
class Frame : public Panel {
    APP_WIDGET(Panel, Frame)

public:
    Frame();
    virtual ~Frame() override;

    virtual void render() override;

    Rect positioned_inner_rect() const;
    Rect relative_inner_rect() const;
    Rect sized_inner_rect() const;

    void set_frame_color(Maybe<Color> c);
    void set_box_style(TInput::TerminalRenderer::BoxStyle box_style);

protected:
    TInput::TerminalRenderer get_renderer_inside_frame();

private:
    Maybe<Color> m_frame_color;
    TInput::TerminalRenderer::BoxStyle m_box_style { TInput::TerminalRenderer::BoxStyle::Thick };
};
}
