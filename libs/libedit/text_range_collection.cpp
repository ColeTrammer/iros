#include <edit/document.h>
#include <edit/text_range_collection.h>

namespace Edit {
TextRangeCollectionIterator TextRangeCollection::iterator(int start_line_index, int start_index_into_line) const {
    int range_index;
    for (range_index = 0; range_index < m_ranges.size(); range_index++) {
        auto& range = m_ranges[range_index];
        if (range.includes(start_line_index, start_index_into_line) || range.ends_before(start_line_index, start_index_into_line)) {
            break;
        }
    }

    return TextRangeCollectionIterator(*this, start_line_index, start_index_into_line, range_index);
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

    if (!range->includes(m_line_index, m_index_into_line)) {
        return {};
    }
    return range->metadata();
}

void TextRangeCollectionIterator::advance() {
    auto& line = m_collection.m_document.line_at_index(m_line_index);
    if (m_index_into_line == line.length()) {
        m_index_into_line = 0;
        m_line_index++;
    } else {
        m_index_into_line++;
    }

    auto* range = current_range();
    if (range && range->ends_before(m_line_index, m_index_into_line)) {
        m_range_index++;
    }
}
}
