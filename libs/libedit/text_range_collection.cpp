#include <assert.h>
#include <edit/document.h>
#include <edit/text_range_collection.h>

namespace Edit {
TextRangeCollectionIterator TextRangeCollection::iterator(const TextIndex& start) const {
    int range_index;
    for (range_index = 0; range_index < m_ranges.size(); range_index++) {
        auto& range = m_ranges[range_index];
        if (range.includes(start) || range.starts_after(start)) {
            break;
        }
    }

    return TextRangeCollectionIterator(*this, start, range_index);
}

const TextRange* TextRangeCollectionIterator::current_range() const {
    if (m_range_index == m_collection.m_ranges.size()) {
        return nullptr;
    }
    return &m_collection.m_ranges[m_range_index];
}

CharacterMetadata TextRangeCollectionIterator::peek_metadata() const {
    auto* range = current_range();
    if (!range) {
        return {};
    }

    if (!range->includes(m_index)) {
        return {};
    }
    return range->metadata();
}

void TextRangeCollectionIterator::advance() {
    auto& line = m_collection.m_document.line_at_index(m_index.line_index());
    if (m_index.index_into_line() == line.length()) {
        m_index.set(m_index.line_index() + 1, 0);
    } else {
        m_index.set_index_into_line(m_index.index_into_line() + 1);
    }

    auto* range = current_range();
    if (range && range->ends_before(m_index)) {
        m_range_index++;
    }
}

void TextRangeCollectionIterator::advance_line() {
    if (m_index.line_index() == m_collection.m_document.num_lines() - 1) {
        return;
    }

    int starting_line_index = m_index.line_index();
    while (m_index.line_index() == starting_line_index) {
        advance();
    }
}

void TextRangeCollectionIterator::advance_to_index_into_line(int index_into_line) {
    assert(m_index.index_into_line() <= index_into_line);
    while (m_index.index_into_line() < index_into_line) {
        advance();
    }
}
}
