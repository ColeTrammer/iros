#include <edit/line_renderer.h>

namespace Edit {
RenderedLine LineRenderer::finish() {
    auto last_index = 0;
    if (!m_rendered_line.position_ranges.empty()) {
        last_index = m_rendered_line.position_ranges.last().index_into_line + 1;
    }
    begin_segment(last_index, 0, PositionRangeType::Normal);
    add_to_segment(" ", 1);
    end_segment();

    if (!m_current_rendered_line.is_empty()) {
        m_rendered_line.rendered_lines.add(move(m_current_rendered_line));
    }
    return move(m_rendered_line);
}

void LineRenderer::begin_segment(int index_into_line, int optional_metadata, PositionRangeType type) {
    m_current_range.start = current_position();
    m_current_range.optional_metadata = optional_metadata;
    m_current_range.index_into_line = index_into_line;
    m_current_range.type = type;
}

void LineRenderer::add_to_segment(const StringView& text, int display_width) {
    if (display_width > m_max_width) {
        display_width = m_max_width;
    }

    if (m_word_wrap_enabled && current_position().col + display_width > m_max_width) {
        m_current_position.row++;
        m_current_position.col = 0;
        m_rendered_line.rendered_lines.add(move(m_current_rendered_line));
    }

    m_current_rendered_line += String(text);
    m_current_position.col += display_width;
    m_absolute_col_position += display_width;

    if (m_word_wrap_enabled && current_position().col >= m_max_width) {
        m_current_position.row++;
        m_current_position.col = 0;
        m_rendered_line.rendered_lines.add(move(m_current_rendered_line));
    }
}

void LineRenderer::end_segment() {
    m_current_range.end = current_position();
    m_rendered_line.position_ranges.add(move(m_current_range));
}
}
