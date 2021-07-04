#pragma once

#include <edit/forward.h>
#include <edit/text_range.h>
#include <liim/vector.h>

namespace Edit {
class TextRangeCollectionIterator;

class TextRangeCollection {
public:
    TextRangeCollection(Document& document) : m_document(document) {}

    void add(const TextRange& range) { m_ranges.add(range); }
    void clear() { m_ranges.clear(); }

    TextRangeCollectionIterator iterator(int start_line_index, int start_index_into_lien) const;

private:
    friend class TextRangeCollectionIterator;

    Vector<TextRange> m_ranges;
    Document& m_document;
};

class TextRangeCollectionIterator {
public:
    TextRangeCollectionIterator(const TextRangeCollection& collection, int start_line_index, int start_index_into_line,
                                int start_range_index)
        : m_collection(collection)
        , m_line_index(start_line_index)
        , m_index_into_line(start_index_into_line)
        , m_range_index(start_range_index) {}

    CharacterMetadata peek_metadata() const;
    void advance();

private:
    const TextRange* current_range() const;

    const TextRangeCollection& m_collection;
    int m_line_index { 0 };
    int m_index_into_line { 0 };
    int m_range_index { 0 };
};
}
