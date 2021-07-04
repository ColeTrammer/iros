#pragma once

#include <edit/character_metadata.h>

namespace Edit {
class TextRange {
public:
    TextRange(int start_line_index, int start_index_into_line, int end_line_index, int end_index_into_line, CharacterMetadata metadata)
        : m_start_line_index(start_line_index)
        , m_start_index_into_line(start_index_into_line)
        , m_end_line_index(end_line_index)
        , m_end_index_into_line(end_index_into_line)
        , m_metadata(metadata) {}

    int start_line_index() const { return m_start_line_index; }
    int start_index_into_line() const { return m_start_index_into_line; }
    int end_line_index() const { return m_end_line_index; }
    int end_index_into_line() const { return m_end_index_into_line; }
    CharacterMetadata metadata() const { return m_metadata; }

    bool includes(int line_index, int index_into_line) const;
    bool starts_after(int line_index, int index_into_line) const;
    bool ends_before(int line_index, int index_into_line) const;

private:
    int m_start_line_index { -1 };
    int m_start_index_into_line { -1 };
    int m_end_line_index { -1 };
    int m_end_index_into_line { -1 };
    CharacterMetadata m_metadata;
};
}
