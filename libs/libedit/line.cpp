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

    auto col_to_relative_position = [&](int col) -> Position {
        if (!document.word_wrap_enabled() || col == 0) {
            return { 0, col };
        }

        auto absolute_row = absolute_row_position(document, panel);
        int relative_row = 0;
        while (col >= panel.cols_at_row(absolute_row + relative_row)) {
            col -= panel.cols_at_row(absolute_row + relative_row);
            relative_row++;
        }
        return { relative_row, col };
    };

    if (index >= length()) {
        if (m_rendered_spans.empty()) {
            return col_to_relative_position(0);
        }
        return col_to_relative_position(m_rendered_spans.last().rendered_end);
    }

    if (index == 0) {
        return col_to_relative_position(0);
    }

    return col_to_relative_position(m_rendered_spans[index - 1].rendered_end);
}

int Line::absoulte_col_offset_of_index(const Document& document, const Panel& panel, int index) const {
    compute_rendered_contents(document, panel);

    int col = 0;
    for (int i = 0; i < index; i++) {
        col += m_rendered_spans[i].rendered_end - m_rendered_spans[i].rendered_start;
    }
    return col;
}

int Line::index_of_relative_position(const Document& document, const Panel& panel, const Position& position) const {
    compute_rendered_contents(document, panel);

    auto absolute_row = absolute_row_position(document, panel);
    int relative_row = 0;
    int accumulated_row_width = 0;
    for (int index = 0; index < length(); index++) {
        auto& span = m_rendered_spans[index];
        auto rendered_line_col = span.rendered_end - accumulated_row_width;
        if (relative_row == position.row && position.col < rendered_line_col) {
            return index;
        }

        if (document.word_wrap_enabled() && rendered_line_col >= panel.cols_at_row(absolute_row + relative_row)) {
            accumulated_row_width += panel.cols_at_row(absolute_row + relative_row);
            relative_row++;
        }
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
    if (!document.word_wrap_enabled()) {
        return 1;
    }

    if (m_rendered_line_count_valid) {
        return m_rendered_line_count;
    }
    m_rendered_line_count_valid = true;

    compute_rendered_contents(document, panel);

    auto absolute_row = absolute_row_position(document, panel);
    int line_count = 0;
    int line_width = m_rendered_contents.size();
    while (line_width > 0) {
        line_width -= panel.cols_at_row(absolute_row + line_count);
        line_count++;
    }

    m_rendered_line_count = max(line_count, 1);
    return m_rendered_line_count;
}

int Line::max_col_in_relative_row(const Document& document, const Panel& panel, int row) const {
    compute_rendered_contents(document, panel);

    auto absolute_row = absolute_row_position(document, panel);
    if (row < rendered_line_count(document, panel) - 1) {
        return panel.cols_at_row(absolute_row + row);
    }

    int accumulated_row_width = 0;
    for (int i = 0; i < row; i++) {
        accumulated_row_width += panel.cols_at_row(absolute_row + i);
    }
    return m_rendered_contents.size() - accumulated_row_width;
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
    if (m_rendered_contents_valid) {
        return;
    }

    m_rendered_contents_valid = true;
    m_rendered_contents.clear();
    m_rendered_spans.resize(length());

    int col_position = 0;
    for (int line_index = 0; line_index <= length(); line_index++) {
        if (document.preview_auto_complete() && document.selection().empty() && this == &document.line_at_cursor() &&
            line_index == document.index_into_line_at_cursor()) {
            auto suggestions = panel.get_suggestions();
            if (suggestions.suggestion_count() == 1) {
                auto& text = suggestions.suggestion_list().first();
                int length_to_write = text.size() - suggestions.suggestion_offset();
                for (int i = 0; i < length_to_write; i++) {
                    m_rendered_contents += String(text[suggestions.suggestion_offset() + i]);
                }
                col_position += length_to_write;
            }
        }

        if (line_index == length()) {
            break;
        }

        char c = char_at(line_index);
        if (c == '\t') {
            int num_spaces = tab_width - (col_position % tab_width);
            for (int i = 0; i < num_spaces; i++) {
                m_rendered_contents += String(' ');
            }
            m_rendered_spans[line_index] = { col_position, col_position + num_spaces };
            col_position += num_spaces;
        } else {
            m_rendered_contents += String(c);
            m_rendered_spans[line_index] = { col_position, col_position + 1 };
            col_position++;
        }
    }
}

int Line::render(const Document& document, Panel& panel, DocumentTextRangeIterator& metadata_iterator, int col_offset,
                 int relative_row_start, int row_in_panel) const {
    compute_rendered_contents(document, panel);

    auto row_count = rendered_line_count(document, panel);
    auto absolute_row = absolute_row_position(document, panel);

    int row = 0;
    int col_position = 0;
    int render_index = 0;
    for (; row + row_in_panel < panel.rows() && row < row_count; row++) {
        int index_into_line = index_of_relative_position(document, panel, { row, col_offset });
        metadata_iterator.advance_to_index_into_line(index_into_line);
        render_index += col_offset;

        col_position = 0;
        while (col_position < panel.cols_at_row(absolute_row + row) && static_cast<size_t>(render_index) < m_rendered_contents.size()) {
            CharacterMetadata metadata(CharacterMetadata::Flags::AutoCompletePreview);
            if (index_into_line < length() && render_index >= m_rendered_spans[index_into_line].rendered_start) {
                metadata = metadata_iterator.peek_metadata();
            }

            if (row >= relative_row_start) {
                panel.set_text_at(row_in_panel + row - relative_row_start, col_position, m_rendered_contents[render_index], metadata);
            }

            col_position++;
            render_index++;
            if (index_into_line < length() && render_index >= m_rendered_spans[index_into_line].rendered_end) {
                index_into_line++;
                metadata_iterator.advance();
            }
        }
    }

    for (; col_position < panel.cols_at_row(absolute_row + row - 1); col_position++) {
        panel.set_text_at(row_in_panel + row - relative_row_start - 1, col_position, ' ', CharacterMetadata());
    }
    metadata_iterator.advance_line();
    return row_count - relative_row_start;
}
}
