#include <assert.h>
#include <edit/document.h>
#include <edit/line.h>
#include <edit/panel.h>
#include <edit/position.h>

namespace Edit {
LineSplitResult Line::split_at(int position) {
    StringView first = StringView(contents().string(), contents().string() + position - 1);
    StringView second = StringView(contents().string() + position);

    auto l1 = Line(String(first));
    auto l2 = Line(String(second));

    return { move(l1), move(l2) };
}

Line::Line(String contents) : m_contents(move(contents)) {}

Line::~Line() {}

Position Line::relative_position_of_index(const Document& document, const Panel& panel, int index) const {
    compute_rendered_contents(document, panel);

    if (m_position_ranges.empty()) {
        return { 0, 0 };
    }

    for (auto& range : m_position_ranges) {
        if (range.index_into_line != index) {
            continue;
        }

        if (range.type == PositionRangeType::InlineBeforeCursor) {
            return range.end;
        }

        return range.start;
    }

    return m_position_ranges.last().end;
}

int Line::absoulte_col_offset_of_index(const Document& document, const Panel& panel, int index) const {
    compute_rendered_contents(document, panel);

    int col = 0;
    for (auto& range : m_position_ranges) {
        if (range.index_into_line == index) {
            break;
        }

        if (range.type != PositionRangeType::Normal) {
            continue;
        }

        auto start = range.start;
        auto end = range.end;
        while (start != end) {
            col++;
            start.col++;
            if (document.word_wrap_enabled() && start.col >= panel.cols()) {
                start.row++;
                start.col = 0;
            }
        }
    }
    return col;
}

int Line::range_index_of_relative_position(const Document& document, const Panel& panel, const Position& position) const {
    compute_rendered_contents(document, panel);

    for (int i = 0; i < m_position_ranges.size(); i++) {
        auto& range = m_position_ranges[i];
        if (position >= range.start && position < range.end) {
            return i;
        }
    }
    return -1;
}

int Line::index_of_relative_position(const Document& document, const Panel& panel, const Position& position) const {
    compute_rendered_contents(document, panel);

    int index = range_index_of_relative_position(document, panel, position);
    if (index != -1) {
        return m_position_ranges[index].index_into_line;
    }
    return length();
}

int Line::absolute_row_position(const Document& document, const Panel& panel) const {
    if (!document.word_wrap_enabled()) {
        return document.index_of_line(*this);
    }

    int absolute_row = 0;
    for (int i = 0; i < document.num_lines(); i++) {
        if (this == &document.line_at_index(i)) {
            return absolute_row;
        }
        absolute_row += document.line_at_index(i).rendered_line_count(document, panel);
    }
    assert(false);
}

int Line::rendered_line_count(const Document& document, const Panel& panel) const {
    compute_rendered_contents(document, panel);
    return m_rendered_lines.size();
}

int Line::max_col_in_relative_row(const Document& document, const Panel& panel, int row) const {
    compute_rendered_contents(document, panel);
    return m_rendered_lines[row].size();
}

void Line::insert_char_at(int position, char c) {
    m_contents.insert(c, position);
    invalidate_rendered_contents();
}

void Line::remove_char_at(int position) {
    m_contents.remove_index(position);
    invalidate_rendered_contents();
}

void Line::combine_line(Line& line) {
    m_contents += line.contents();
    invalidate_rendered_contents();
}

void Line::search(const Document& document, const String& text, TextRangeCollection& results) {
    auto line_index = document.index_of_line(*this);
    int index_into_line = 0;
    for (;;) {
        char* match = strstr(m_contents.string() + index_into_line, text.string());
        if (!match) {
            break;
        }

        index_into_line = match - m_contents.string();
        results.add({ { line_index, index_into_line },
                      { line_index, index_into_line + static_cast<int>(text.size()) },
                      { CharacterMetadata::Flags::Highlighted } });
        index_into_line += text.size();
    }
}

void Line::compute_rendered_contents(const Document& document, const Panel& panel) const {
    if (!m_rendered_lines.empty()) {
        return;
    }

    PositionRange current_range;
    String current_segment;
    int current_row = 0;
    int current_col = 0;
    int col_position = 0;

    auto begin_range = [&](int index_into_line, int optional_metadata, PositionRangeType type) {
        current_range.start = { current_row, current_col };
        current_range.optional_metadata = optional_metadata;
        current_range.index_into_line = index_into_line;
        current_range.type = type;
    };

    auto put_char = [&](char c) {
        current_segment += String(c);
        current_col++;
        col_position++;
        if (document.word_wrap_enabled() && current_col >= panel.cols()) {
            current_row++;
            current_col = 0;
            m_rendered_lines.add(move(current_segment));
        }
    };

    auto end_range = [&] {
        current_range.end = { current_row, current_col };
        m_position_ranges.add(current_range);
    };

    auto injected_text = panel.inject_inline_text_for_line_index(document.index_of_line(*this));
    if (!injected_text.is_empty()) {
        begin_range(0, 0, PositionRangeType::InlineBeforeCursor);
        for (auto c : injected_text) {
            put_char(c);
        }
        end_range();
    }

    for (int index_into_line = 0; index_into_line < length(); index_into_line++) {
        begin_range(index_into_line, 0, PositionRangeType::Normal);

        char c = char_at(index_into_line);
        if (c == '\t') {
            int num_spaces = tab_width - (col_position % tab_width);
            for (int i = 0; i < num_spaces; i++) {
                put_char(' ');
            }
        } else {
            put_char(c);
        }

        end_range();

        if (document.preview_auto_complete() && document.selection().empty() && this == &document.line_at_cursor() &&
            index_into_line == document.index_into_line_at_cursor() - 1) {
            auto suggestions = panel.get_suggestions();
            if (suggestions.suggestion_count() == 1) {
                auto& text = suggestions.suggestion_list().first();
                int length_to_write = text.size() - suggestions.suggestion_offset();

                begin_range(index_into_line + 1, CharacterMetadata::Flags::AutoCompletePreview, PositionRangeType::InlineAfterCursor);
                for (int i = 0; i < length_to_write; i++) {
                    put_char(text[suggestions.suggestion_offset() + i]);
                }
                end_range();
            }
        }
    }

    if (m_rendered_lines.empty() || !current_segment.is_empty()) {
        m_rendered_lines.add(move(current_segment));
    }
}

int Line::render(const Document& document, Panel& panel, DocumentTextRangeIterator& metadata_iterator, int col_offset,
                 int relative_row_start, int row_in_panel) const {
    compute_rendered_contents(document, panel);

    auto row_count = rendered_line_count(document, panel);
    for (int row = relative_row_start; row + row_in_panel - relative_row_start < panel.rows() && row < row_count; row++) {
        auto current_panel_position = Position { row, 0 };
        auto render_position = Position { row, col_offset };
        auto range_index = range_index_of_relative_position(document, panel, render_position);
        if (range_index != -1) {
            int index_into_line = m_position_ranges[range_index].index_into_line;
            metadata_iterator.advance_to_index_into_line(index_into_line);

            auto& rendered_line = m_rendered_lines[row];
            while (current_panel_position.col < panel.cols() && static_cast<size_t>(render_position.col) < rendered_line.size()) {
                auto& current_range = m_position_ranges[range_index];
                auto metadata = CharacterMetadata { current_range.optional_metadata };
                if (current_range.type == PositionRangeType::Normal) {
                    metadata = metadata_iterator.peek_metadata();
                }

                panel.set_text_at(row_in_panel + row - relative_row_start, current_panel_position.col, rendered_line[render_position.col],
                                  metadata);

                current_panel_position.col++;
                render_position.col++;
                if (render_position >= current_range.end) {
                    if (current_range.type == PositionRangeType::Normal) {
                        index_into_line++;
                        metadata_iterator.advance();
                    }
                    range_index++;
                }
            }
        }

        for (; current_panel_position.col < panel.cols(); current_panel_position.col++) {
            panel.set_text_at(row_in_panel + row - relative_row_start, current_panel_position.col, ' ', CharacterMetadata());
        }
    }

    metadata_iterator.advance_line();
    return row_count - relative_row_start;
}
}
