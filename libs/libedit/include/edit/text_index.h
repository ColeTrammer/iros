#pragma once

namespace Edit {
class TextIndex {
public:
    TextIndex(int line_index, int index_into_line) : m_line_index(line_index), m_index_into_line(index_into_line) {}

    int line_index() const { return m_line_index; }
    int index_into_line() const { return m_index_into_line; }

    void set_line_index(int index) { m_line_index = index; }
    void set_index_into_line(int index) { m_index_into_line = index; }
    void set(int line_index, int index_into_line) {
        m_line_index = line_index;
        m_index_into_line = index_into_line;
    }

    bool operator==(const TextIndex& other) const {
        return this->line_index() == other.line_index() && this->index_into_line() == other.index_into_line();
    }
    bool operator!=(const TextIndex& other) const { return !(*this == other); }

    bool is_before(const TextIndex& other) const {
        if (this->line_index() < other.line_index()) {
            return true;
        }

        if (this->line_index() == other.line_index()) {
            return this->index_into_line() < other.index_into_line();
        }
        return false;
    }

    bool is_after(const TextIndex& other) const {
        if (this->line_index() > other.line_index()) {
            return true;
        }

        if (this->line_index() == other.line_index()) {
            return this->index_into_line() > other.index_into_line();
        }
        return false;
    }

private:
    int m_line_index { 0 };
    int m_index_into_line { 0 };
};
}
