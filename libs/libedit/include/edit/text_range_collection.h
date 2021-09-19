#pragma once

#include <edit/forward.h>
#include <edit/text_range.h>
#include <liim/fixed_array.h>
#include <liim/vector.h>

namespace Edit {
class TextRangeCollectionIterator;

class TextRangeCollection {
public:
    TextRangeCollection() = default;

    void add(const TextRange& range) { m_ranges.add(range); }
    void clear() { m_ranges.clear(); }
    int size() const { return m_ranges.size(); }
    bool empty() const { return m_ranges.empty(); }
    const TextRange& range(int index) const { return m_ranges[index]; }

    void sort();
    TextRangeCollectionIterator iterator(const TextIndex& start) const;
    Maybe<TextRange> range_at_text_index(const TextIndex& index) const;
    Maybe<int> range_index_at_text_index(const TextIndex& index) const;

private:
    friend class TextRangeCollectionIterator;

    Vector<TextRange> m_ranges;
};

class TextRangeCollectionIterator {
public:
    TextRangeCollectionIterator(const TextRangeCollection& collection, const TextIndex& start, int start_range_index)
        : m_collection(collection), m_index(start), m_range_index(start_range_index) {}

    CharacterMetadata peek_metadata() const;
    void advance(const Document& document);
    void advance_line(const Document& document);
    void advance_to_index_into_line(const Document& document, int index_into_line);

private:
    const TextRange* current_range() const;

    const TextRangeCollection& m_collection;
    TextIndex m_index;
    int m_range_index { 0 };
};

template<size_t N>
class TextRangeCombinerIterator {
public:
    template<typename... TextRangeCollections>
    TextRangeCombinerIterator(const TextIndex& start, TextRangeCollections&&... collections)
        : m_iterators { forward<TextRangeCollections>(collections).iterator(start)... } {}

    CharacterMetadata peek_metadata() const {
        CharacterMetadata metadata;
        for (auto& iter : m_iterators) {
            metadata = metadata.combine(iter.peek_metadata());
        }
        return metadata;
    }

    void advance(const Document& document) {
        for (auto& iter : m_iterators) {
            iter.advance(document);
        }
    }

    void advance_line(const Document& document) {
        for (auto& iter : m_iterators) {
            iter.advance_line(document);
        }
    }

    void advance_to_index_into_line(const Document& document, int index_into_line) {
        for (auto& iter : m_iterators) {
            iter.advance_to_index_into_line(document, index_into_line);
        }
    }

private:
    TextRangeCollectionIterator m_iterators[N];
};
}
