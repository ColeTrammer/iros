#include <edit/display.h>
#include <edit/document.h>
#include <edit/rendered_line.h>

namespace Edit {
bool RenderedLine::is_up_to_date() const {
    return !m_rendered_lines.empty();
}

void RenderedLine::invalidate() {
    m_rendered_lines.clear();
    m_position_ranges.clear();
}

void RenderedLine::append_line(String&& string, Vector<PositionRange>&& ranges) {
    m_rendered_lines.add(move(string));
    m_position_ranges.add(move(ranges));
}

RelativePosition RenderedLine::relative_position_of_index(const TextIndex& index) const {
    auto* range = range_for_index(index, RangeFor::Cursor);
    if (!range) {
        assert(!m_position_ranges.empty());
        return m_position_ranges.last().last().end;
    }
    return range->start;
}

int RenderedLine::absolute_col_offset_of_index(const TextIndex& index) const {
    auto* range = range_for_index(index, RangeFor::Text);
    if (!range) {
        return 0;
    }

    return range->start_absolute_col;
}

TextIndex RenderedLine::index_of_absolute_position(const AbsolutePosition& position) const {
    auto* range = range_for_relative_position({ position.relative_row(), position.relative_col() });
    if (!range) {
        return { position.line_index(), m_position_ranges.last().last().index_into_line };
    }
    return { position.line_index(), range->index_into_line };
}

int RenderedLine::rendered_line_count() const {
    return m_rendered_lines.size();
}

TextIndex RenderedLine::next_index_into_line(const TextIndex& index) const {
    auto* last_range = static_cast<const PositionRange*>(nullptr);
    for (int row = m_position_ranges.size() - 1; row >= 0; row--) {
        auto& position_ranges = m_position_ranges[row];
        for (int i = position_ranges.size() - 1; i >= 0; i--) {
            auto& range = position_ranges[i];
            if (range.index_into_line > index.index_into_line() && range.type == PositionRangeType::Normal) {
                last_range = &range;
            }
        }
    }
    return { index.line_index(), last_range ? last_range->index_into_line : m_position_ranges.last().last().index_into_line };
}

TextIndex RenderedLine::prev_index_into_line(const TextIndex& index) const {
    assert(index.index_into_line() != 0);

    auto* last_range = static_cast<const PositionRange*>(nullptr);
    for (auto& position_ranges : m_position_ranges) {
        for (auto& range : position_ranges) {
            if (range.index_into_line < index.index_into_line() && range.type == PositionRangeType::Normal) {
                last_range = &range;
            }
        }
    }
    return { index.line_index(), last_range ? last_range->index_into_line : 0 };
}

void RenderedLine::update_metadata(Display& display, const TextIndex& this_line_start) {
    auto cursor_collection = display.cursors().cursor_text_ranges();
    auto selection_collection = display.cursors().selections();
    auto metadata_iterator = Edit::DocumentTextRangeIterator { this_line_start, display.document()->syntax_highlighting_info(),
                                                               display.search_results(), cursor_collection, selection_collection };

    for (int row = 0; row < m_position_ranges.size(); row++) {
        for (auto& range : m_position_ranges[row]) {
            if (range.type == PositionRangeType::Normal) {
                metadata_iterator.advance_to_index_into_line(*display.document(), range.index_into_line);
                range.metadata = metadata_iterator.peek_metadata();
            }
        }
    }
}

int RenderedLine::render(Display& display, const TextIndex& this_line_start, int col_offset, int relative_row_start, int row_in_display) {
    // FIXME: this could be done only when the metadata is considered to be invalidated.
    update_metadata(display, this_line_start);

    auto row_count = rendered_line_count();
    for (int row = relative_row_start; row + row_in_display - relative_row_start < display.rows() && row < row_count; row++) {
        display.output_line(row + row_in_display - relative_row_start, col_offset, *this, row);
    }

    return row_count - relative_row_start;
}

const PositionRange* RenderedLine::range_for_relative_position(const RelativePosition& position) const {
    if (position.row() < 0 || position.row() >= m_position_ranges.size()) {
        return nullptr;
    }

    for (auto& range : m_position_ranges[position.row()]) {
        if (position >= range.start && position < range.end) {
            return &range;
        }
    }
    return nullptr;
}

const PositionRange* RenderedLine::range_for_index(const TextIndex& index, RangeFor mode) const {
    for (auto& position_ranges : m_position_ranges) {
        for (auto& range : position_ranges) {
            if (range.index_into_line >= index.index_into_line()) {
                if (mode == RangeFor::Text && range.type == PositionRangeType::Normal) {
                    return &range;
                }
                if (mode == RangeFor::Cursor &&
                    (range.type == PositionRangeType::Normal || range.type == PositionRangeType::InlineAfterCursor)) {
                    return &range;
                }
            }
        }
    }
    return nullptr;
}
}
