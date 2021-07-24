#include <edit/line_renderer.h>

namespace Edit {
RenderedLine LineRenderer::finish(const Line& line) {
    begin_segment(line.length(), 0, PositionRangeType::Normal);
    add_to_segment(" ", 1);
    end_segment();

    if (!m_current_rendered_line.empty() || !m_current_position_ranges.empty()) {
        m_rendered_line.rendered_lines.add(move(m_current_rendered_line));
        m_rendered_line.position_ranges.add(move(m_current_position_ranges));
    }
    return move(m_rendered_line);
}

void LineRenderer::begin_segment(int index_into_line, int optional_metadata, PositionRangeType type) {
    m_current_range.start = current_position();
    m_current_range.optional_metadata = optional_metadata;
    m_current_range.index_into_line = index_into_line;
    m_current_range.type = type;
    m_current_range.byte_count_in_rendered_string = 0;
    m_current_range.start_absolute_col = m_absolute_col_position;
}

void LineRenderer::add_to_segment(const StringView& text, int display_width) {
    if (display_width > m_max_width) {
        display_width = m_max_width;
    }

    if (m_word_wrap_enabled && current_position().col + display_width > m_max_width) {
        m_current_position.row++;
        m_current_position.col = 0;
        m_rendered_line.rendered_lines.add(move(m_current_rendered_line));
        m_rendered_line.position_ranges.add(move(m_current_position_ranges));
        m_current_range.start = current_position();
    }

    m_current_range.byte_count_in_rendered_string += text.size();
    m_current_rendered_line += String(text);
    m_current_position.col += display_width;
    m_absolute_col_position += display_width;
}

void LineRenderer::end_segment() {
    m_current_range.end = current_position();
    m_current_position_ranges.add(move(m_current_range));
}
}
