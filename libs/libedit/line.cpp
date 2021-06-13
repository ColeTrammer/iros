#include <assert.h>
#include <edit/line.h>
#include <edit/panel.h>

LineSplitResult Line::split_at(int position) {
    StringView first = StringView(contents().string(), contents().string() + position - 1);
    StringView second = StringView(contents().string() + position);

    auto l1 = Line(String(first));
    auto l2 = Line(String(second));

    for (int i = 0; i < position; i++) {
        l1.metadata_at(i) = move(metadata_at(i));
    }

    for (int i = position; i < length(); i++) {
        l2.metadata_at(i - position) = move(metadata_at(i));
    }

    return { move(l1), move(l2) };
}

Line::Line(String contents) : m_contents(move(contents)) {
    m_metadata.resize(m_contents.size());
    assert(m_contents.size() == static_cast<size_t>(m_metadata.size()));
}

Line::~Line() {}

int Line::col_position_of_index(int index) const {
    int col = 0;
    for (int i = 0; i < index; i++) {
        char c = contents()[i];
        if (c == '\t') {
            col += tab_width - (col % tab_width);
        } else {
            col++;
        }
    }
    return col;
}

int Line::index_of_col_position(int position) const {
    int col = 0;
    int index;
    for (index = 0; index < length(); index++) {
        char c = contents()[index];
        int col_width = 1;
        if (c == '\t') {
            col_width = tab_width - (col % tab_width);
        }

        col += col_width;
        if (col > position) {
            break;
        }
    }
    return index;
}

void Line::insert_char_at(int position, char c) {
    assert(m_contents.size() == static_cast<size_t>(m_metadata.size()));
    m_contents.insert(c, position);
    m_metadata.insert(CharacterMetadata(), position);
}

void Line::remove_char_at(int position) {
    m_contents.remove_index(position);
    m_metadata.remove(position);
}

void Line::combine_line(Line& line) {
    m_contents += line.contents();
    m_metadata.add(move(line.metadata()));
    assert(static_cast<size_t>(m_metadata.size()) == m_contents.size());
}

void Line::clear_search() {
    for (auto& m : m_metadata) {
        m.set_highlighted(false);
    }
}

void Line::clear_selection() {
    for (auto& m : m_metadata) {
        m.set_selected(false);
    }
}

void Line::toggle_select_after(int index) {
    for (int i = index; i < length(); i++) {
        metadata_at(i).invert_selected();
    }
}

void Line::toggle_select_before(int index) {
    for (int i = 0; i < index; i++) {
        metadata_at(i).invert_selected();
    }
}

void Line::select_all() {
    for (auto& m : m_metadata) {
        m.set_selected(true);
    }
}

int Line::search(const String& text) {
    char* s = m_contents.string();
    int matches = 0;
    for (;;) {
        char* match = strstr(s, text.string());
        if (!match) {
            break;
        }

        for (size_t i = match - m_contents.string(); i < match - m_contents.string() + text.size(); i++) {
            metadata_at(i).set_highlighted(true);
        }

        s = match + text.size();
        matches++;
    }

    return matches;
}

void Line::clear_syntax_highlighting() {
    for (auto& m : m_metadata) {
        m.clear_syntax_highlighting();
    }
}

void Line::render(Panel& panel, int col_offset, int row_in_panel) const {
    int col_position = 0;
    int line_index = index_of_col_position(col_offset);
    while (line_index < length() && col_position < panel.cols_at_row(row_in_panel)) {
        char c = char_at(line_index);
        auto metadata = metadata_at(line_index);
        if (c == '\t') {
            int num_spaces = tab_width - ((col_position + col_offset) % tab_width);
            for (int i = 0; col_position < panel.cols_at_row(row_in_panel) && i < num_spaces; i++) {
                panel.set_text_at(row_in_panel, col_position++, ' ', metadata);
            }
        } else {
            panel.set_text_at(row_in_panel, col_position++, c, metadata);
        }

        line_index++;
    }

    for (; col_position < panel.cols_at_row(row_in_panel); col_position++) {
        panel.set_text_at(row_in_panel, col_position, ' ', CharacterMetadata());
    }
}
