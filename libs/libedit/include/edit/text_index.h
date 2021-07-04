#pragma once

namespace Edit {
class TextIndex {
public:
    TextIndex(int line_index, int index_into_line) : m_line_index(line_index), m_index_into_line(index_into_line) {}

    int line_index() const { return m_line_index; }
    int index_into_line() const { return m_index_into_line; }

    void set_line_index(int index) { m_line_index = index; }
    void set_index_into_line(int index) { m_index_into_line = index; }

private:
    int m_line_index { 0 };
    int m_index_into_line { 0 };
};
}
