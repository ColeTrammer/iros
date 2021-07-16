#include <assert.h>
#include <edit/character_metadata.h>
#include <edit/document.h>
#include <edit/line.h>
#include <edit/panel.h>
#include <edit/position.h>
#include <edit/rendered_line.h>

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

void Line::overwrite(Document& document, Line&& line) {
    auto old_length = this->length();
    auto delta_length = line.length() - old_length;
    this->m_contents = move(line.contents());
    if (delta_length < 0) {
        document.panel().cursors().did_delete_from_line(document, document.index_of_line(*this), old_length, -delta_length);
    } else if (delta_length > 0) {
        document.panel().cursors().did_add_to_line(document, document.index_of_line(*this), old_length, delta_length);
    }
    invalidate_rendered_contents(document, document.panel());
}

Position Line::relative_position_of_index(const Document& document, Panel& panel, int index) const {
    auto& info = compute_rendered_contents(document, panel);

    auto* range = range_for_index_into_line(document, panel, index, RangeFor::Cursor);
    if (!range) {
        assert(!info.position_ranges.empty());
        return info.position_ranges.last().last().end;
    }
    return range->start;
}

int Line::absoulte_col_offset_of_index(const Document& document, Panel& panel, int index) const {
    auto* range = range_for_index_into_line(document, panel, index, RangeFor::Text);
    if (!range) {
        return 0;
    }

    return range->start_absolute_col + (range->end.col - range->start.col);
}

const PositionRange* Line::range_for_index_into_line(const Document& document, Panel& panel, int index_into_line, RangeFor mode) const {
    auto& info = compute_rendered_contents(document, panel);

    for (auto& position_ranges : info.position_ranges) {
        for (auto& range : position_ranges) {
            if (range.index_into_line == index_into_line) {
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

const PositionRange* Line::range_for_relative_position(const Document& document, Panel& panel, const Position& position) const {
    auto& info = compute_rendered_contents(document, panel);

    if (position.row < 0 || position.row >= info.position_ranges.size()) {
        return nullptr;
    }

    for (auto& range : info.position_ranges[position.row]) {
        if (position >= range.start && position < range.end) {
            return &range;
        }
    }
    return nullptr;
}

int Line::index_of_relative_position(const Document& document, Panel& panel, const Position& position) const {
    auto* range = range_for_relative_position(document, panel, position);
    if (!range) {
        return length();
    }
    return range->index_into_line;
}

int Line::absolute_row_position(const Document& document, Panel& panel) const {
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

int Line::rendered_line_count(const Document& document, Panel& panel) const {
    auto& info = compute_rendered_contents(document, panel);
    return info.rendered_lines.size();
}

int Line::max_col_in_relative_row(const Document& document, Panel& panel, int row) const {
    auto& info = compute_rendered_contents(document, panel);
    return info.position_ranges[row].last().end.col;
}

void Line::insert_char_at(Document& document, int position, char c) {
    m_contents.insert(c, position);
    document.panel().cursors().did_add_to_line(document, document.index_of_line(*this), position, 1);
    invalidate_rendered_contents(document, document.panel());
}

void Line::remove_char_at(Document& document, int position) {
    m_contents.remove_index(position);
    document.panel().cursors().did_delete_from_line(document, document.index_of_line(*this), position, 1);
    invalidate_rendered_contents(document, document.panel());
}

void Line::combine_line(Document& document, Line& line) {
    auto old_length = length();
    m_contents += line.contents();
    document.panel().cursors().did_add_to_line(document, document.index_of_line(*this), old_length, line.length());
    invalidate_rendered_contents(document, document.panel());
}

void Line::search(const Document& document, const String& text, TextRangeCollection& results) const {
    auto line_index = document.index_of_line(*this);
    int index_into_line = 0;
    for (;;) {
        const char* match = strstr(m_contents.string() + index_into_line, text.string());
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

void Line::invalidate_rendered_contents(const Document& document, Panel& panel) const {
    auto& info = panel.rendered_line_at_index(document.index_of_line(*this));
    info.rendered_lines.clear();
    info.position_ranges.clear();
}

const RenderedLine& Line::compute_rendered_contents(const Document& document, Panel& panel) const {
    auto& rendered_line = panel.rendered_line_at_index(document.index_of_line(*this));
    if (!rendered_line.rendered_lines.empty()) {
        return rendered_line;
    }

    rendered_line = panel.compose_line(*this);
    return rendered_line;
}

int Line::render(const Document& document, Panel& panel, DocumentTextRangeIterator& metadata_iterator, int col_offset,
                 int relative_row_start, int row_in_panel) const {
    auto& info = compute_rendered_contents(document, panel);

    auto row_count = rendered_line_count(document, panel);
    for (int row = relative_row_start; row + row_in_panel - relative_row_start < panel.rows() && row < row_count; row++) {
        auto& position_ranges = info.position_ranges[row];
        int range_index = 0;
        int index_into_line = position_ranges[range_index].index_into_line;
        metadata_iterator.advance_to_index_into_line(index_into_line);

        auto& rendered_line = info.rendered_lines[row];
        auto metadata_vector = Vector<CharacterMetadata>(rendered_line.size());
        for (; range_index < position_ranges.size(); range_index++) {
            auto& current_range = position_ranges[range_index];
            auto metadata = CharacterMetadata { current_range.optional_metadata };
            if (current_range.type == PositionRangeType::Normal) {
                metadata = metadata_iterator.peek_metadata();
                metadata_iterator.advance();
            }

            for (int i = 0; i < current_range.byte_count_in_rendered_string; i++) {
                metadata_vector.add(metadata);
            }
        }

        assert(rendered_line.size() == static_cast<size_t>(metadata_vector.size()));
        panel.output_line(row + row_in_panel - relative_row_start, col_offset, rendered_line.view(), metadata_vector);
    }

    metadata_iterator.advance_line();
    return row_count - relative_row_start;
}
}
