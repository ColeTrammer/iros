#pragma once

#include <edit/text_range.h>
#include <liim/utilities.h>

namespace Edit {
class Selection {
public:
    Selection() {}

    void clear() { begin({ 0, 0 }); }
    bool empty() const { return start() == end(); }

    const TextIndex& start() const { return m_start; }
    const TextIndex& end() const { return m_end; }

    void set_start_line_index(int line_index) { set_start({ line_index, start().index_into_line() }); }
    void set_start_index_into_line(int index_into_line) { set_start({ start().line_index(), index_into_line }); }

    void set_end_line_index(int line_index) { set_end({ line_index, end().index_into_line() }); }
    void set_end_index_into_line(int index_into_line) { set_end({ end().line_index(), index_into_line }); }

    void set_start(const TextIndex& start) { set(start, m_end); }
    void set_end(const TextIndex& end) { set(m_start, end); }
    void set(const TextIndex& start, const TextIndex& end) {
        m_start = start;
        m_end = end;
    }

    void begin(const TextIndex& start) { m_start = m_end = start; }

    bool overlaps(const Selection& other) const;
    void merge(const Selection& other);

    const TextIndex& normalized_start() const { return m_start <= m_end ? m_start : m_end; }
    const TextIndex& normalized_end() const { return m_start <= m_end ? m_end : m_start; }

    TextRange text_range() const { return { normalized_start(), normalized_end(), { CharacterMetadata::Flags::Selected } }; }

private:
    TextIndex m_start;
    TextIndex m_end;
};
}
