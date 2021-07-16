#pragma once

#include <edit/line.h>
#include <edit/rendered_line.h>
#include <liim/forward.h>
#include <liim/string.h>

namespace Edit {
class LineRenderer {
public:
    LineRenderer(int max_width, bool word_wrap_enabled) : m_max_width(max_width), m_word_wrap_enabled(word_wrap_enabled) {}

    RenderedLine finish(const Line& line);

    void begin_segment(int index_into_line, int optional_metadata, PositionRangeType type);
    void add_to_segment(const StringView& text, int display_width);
    void end_segment();

    int absolute_col_position() const { return m_absolute_col_position; }
    const Position& current_position() const { return m_current_position; }

private:
    RenderedLine m_rendered_line;
    String m_current_rendered_line;
    Vector<PositionRange> m_current_position_ranges;
    PositionRange m_current_range;
    Position m_current_position;
    int m_absolute_col_position { 0 };
    int m_max_width { 0 };
    bool m_word_wrap_enabled { false };
};
}
