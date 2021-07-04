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

Position Line::position_of_index(const Document& document, const Panel& panel, int index) const {
    compute_rendered_contents(document, panel);

    auto index_of_line = document.index_of_line(*this);
    if (index >= length()) {
        if (m_rendered_spans.empty()) {
            return { index_of_line, 0 };
        }
        return { index_of_line, m_rendered_spans.last().rendered_end };
    }

    if (index == 0) {
        return { index_of_line, 0 };
    }

    return { index_of_line, m_rendered_spans[index - 1].rendered_end };
}

int Line::index_of_position(const Document& document, const Panel& panel, const Position& position) const {
    compute_rendered_contents(document, panel);

    for (int index = 0; index < length(); index++) {
        if (position.col < m_rendered_spans[index].rendered_end) {
            return index;
        }
    }
    return length();
}

void Line::insert_char_at(int position, char c) {
    m_contents.insert(c, position);
}

void Line::remove_char_at(int position) {
    m_contents.remove_index(position);
}

void Line::combine_line(Line& line) {
    m_contents += line.contents();
}

int Line::search(const String& text) {
    char* s = m_contents.string();
    int matches = 0;
    for (;;) {
        char* match = strstr(s, text.string());
        if (!match) {
            break;
        }

        s = match + text.size();
        matches++;
    }

    return matches;
}

void Line::compute_rendered_contents(const Document& document, const Panel& panel) const {
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

void Line::render(const Document& document, Panel& panel, DocumentTextRangeIterator& metadata_iterator, int col_offset,
                  int row_in_panel) const {
    compute_rendered_contents(document, panel);

    int index_into_line = index_of_position(document, panel, { document.index_of_line(*this), col_offset });
    metadata_iterator.advance_to_index_into_line(index_into_line);

    int col_position = 0;
    while (col_position < panel.cols_at_row(row_in_panel) && static_cast<size_t>(col_offset + col_position) < m_rendered_contents.size()) {
        CharacterMetadata metadata(CharacterMetadata::Flags::AutoCompletePreview);
        if (index_into_line < length() && col_position + col_offset >= m_rendered_spans[index_into_line].rendered_start) {
            metadata = metadata_iterator.peek_metadata();
        }
        panel.set_text_at(row_in_panel, col_position, m_rendered_contents[col_offset + col_position], metadata);

        col_position++;
        if (index_into_line < length() && col_position + col_offset >= m_rendered_spans[index_into_line].rendered_end) {
            index_into_line++;
            metadata_iterator.advance();
        }
    }

    for (; col_position < panel.cols_at_row(row_in_panel); col_position++) {
        panel.set_text_at(row_in_panel, col_position, ' ', CharacterMetadata());
    }
    metadata_iterator.advance_line();
}
}
