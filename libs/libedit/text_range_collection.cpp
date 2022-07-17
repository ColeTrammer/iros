#include <assert.h>
#include <edit/document.h>
#include <edit/text_range_collection.h>

namespace Edit {
void TextRangeCollection::sort() {
    ::sort(transform(m_ranges, &TextRange::start));
}

Option<int> TextRangeCollection::range_index_at_text_index(const TextIndex& index) const {
    for (int i = 0; i < m_ranges.size(); i++) {
        if (m_ranges[i].includes(index)) {
            return i;
        }
    }
    return {};
}

Option<TextRange> TextRangeCollection::range_at_text_index(const TextIndex& index) const {
    for (auto& range : m_ranges) {
        if (range.includes(index)) {
            return range;
        }
    }
    return {};
}

TextRangeCollectionIterator TextRangeCollection::iterator(const TextIndex& start) const {
    // Perform binary search to find the first range that starts_after(start) or includes(start).
    int left = 0;
    int right = m_ranges.size();
    while (left < right) {
        auto mid = (left + right) / 2;
        auto& range = m_ranges[mid];
        if (range.includes(start)) {
            left = mid;
            break;
        }
        if (range.starts_after(start)) {
            right = mid;
            continue;
        }
        left = mid + 1;
    }

    return TextRangeCollectionIterator(*this, start, left);
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

void TextRangeCollectionIterator::advance(const Document& document) {
    auto& line = document.line_at_index(m_index.line_index());
    if (m_index.index_into_line() > line.length()) {
        m_index.set(m_index.line_index() + 1, 0);
    } else {
        m_index.set_index_into_line(m_index.index_into_line() + 1);
    }

    auto* range = current_range();
    if (range && range->ends_before(m_index)) {
        m_range_index++;
    }
}

void TextRangeCollectionIterator::advance_line(const Document& document) {
    if (m_index.line_index() == document.last_line_index()) {
        return;
    }

    int starting_line_index = m_index.line_index();
    while (m_index.line_index() == starting_line_index) {
        advance(document);
    }
}

void TextRangeCollectionIterator::advance_to_index_into_line(const Document& document, int index_into_line) {
    assert(m_index.index_into_line() <= index_into_line);
    while (m_index.index_into_line() < index_into_line) {
        advance(document);
    }
}
}
