#pragma once

#include <graphics/color.h>
#include <graphics/point.h>
#include <graphics/rect.h>
#include <graphics/text_align.h>
#include <liim/forward.h>
#include <tinput/forward.h>
#include <tinput/terminal_text_style.h>

namespace TInput {
class TerminalRenderer {
public:
    TerminalRenderer(IOTerminal& io_terminal) : m_io_terminal(io_terminal) {}

    void set_origin(const Point& point) { m_origin = point; }
    void set_clip_rect(const Rect& rect) { m_clip_rect = rect; }

    void clear_rect(const Rect& rect, Maybe<Color> color = {});
    void render_text(const Rect& rect, const StringView& text, const TerminalTextStyle& style = {},
                     TextAlign alignment = TextAlign::CenterLeft);

private:
    Rect translate(const Rect& rect) { return rect.translated(m_origin); }
    Rect constrained(const Rect& rect) { return rect.intersection_with(m_clip_rect); }

    bool in_bounds(const Point& point) { return m_clip_rect.intersects(point); }

    IOTerminal& m_io_terminal;
    Rect m_clip_rect;
    Point m_origin;
};
}
