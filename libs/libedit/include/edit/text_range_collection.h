#pragma once

#include <edit/forward.h>
#include <edit/text_range.h>
#include <liim/fixed_array.h>
#include <liim/vector.h>

namespace Edit {
class TextRangeCollectionIterator;

class TextRangeCollection {
public:
    TextRangeCollection(const Document& document) : m_document(document) {}

    void add(const TextRange& range) { m_ranges.add(range); }
    void clear() { m_ranges.clear(); }

    TextRangeCollectionIterator iterator(int start_line_index, int start_index_into_line) const;

private:
    friend class TextRangeCollectionIterator;

    Vector<TextRange> m_ranges;
    const Document& m_document;
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
    void advance_line();
    void advance_to_index_into_line(int index_into_line);

private:
    const TextRange* current_range() const;

    const TextRangeCollection& m_collection;
    int m_line_index { 0 };
    int m_index_into_line { 0 };
    int m_range_index { 0 };
};

template<size_t N>
class TextRangeCombinerIterator {
public:
    template<typename... TextRangeCollections>
    TextRangeCombinerIterator(int start_line_index, int start_index_into_line, TextRangeCollections&&... collections)
        : m_iterators { forward<TextRangeCollections>(collections).iterator(start_line_index, start_index_into_line)... } {}

    CharacterMetadata peek_metadata() const {
        CharacterMetadata metadata;
        for (auto& iter : m_iterators) {
            metadata = metadata.combine(iter.peek_metadata());
        }
        return metadata;
    }

    void advance() {
        for (auto& iter : m_iterators) {
            iter.advance();
        }
    }

    void advance_line() {
        for (auto& iter : m_iterators) {
            iter.advance_line();
        }
    }

    void advance_to_index_into_line(int index_into_line) {
        for (auto& iter : m_iterators) {
            iter.advance_to_index_into_line(index_into_line);
        }
    }

private:
    TextRangeCollectionIterator m_iterators[N];
};
}
