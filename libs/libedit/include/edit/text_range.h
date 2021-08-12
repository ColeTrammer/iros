#pragma once

#include <edit/character_metadata.h>
#include <edit/text_index.h>

namespace Edit {
class TextRange {
public:
    TextRange(const TextIndex& start, const TextIndex& end, CharacterMetadata metadata, int private_data = 0)
        : m_start(start), m_end(end), m_metadata(metadata), m_private_data(private_data) {}

    const TextIndex& start() const { return m_start; }
    const TextIndex& end() const { return m_end; }

    CharacterMetadata metadata() const { return m_metadata; }
    int private_data() const { return m_private_data; }

    bool includes(const TextIndex& index) const;
    bool starts_after(const TextIndex& index) const;
    bool ends_before(const TextIndex& index) const;

private:
    TextIndex m_start;
    TextIndex m_end;
    CharacterMetadata m_metadata;
    int m_private_data { 0 };
};
}
