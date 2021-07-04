#pragma once

#include <assert.h>
#include <edit/forward.h>
#include <stddef.h>

namespace Edit {
class Cursor {
public:
    Cursor(Document& document) : m_document(document) {}
    Cursor(const Cursor& other) = default;

    Cursor& operator=(const Cursor& other) {
        assert(&this->m_document == &other.m_document);
        set(other.line_index(), other.index_into_line());
        return *this;
    }

    Line& referenced_line() const;
    char referenced_character() const;

    int line_index() const { return m_line_index; }
    int index_into_line() const { return m_index_into_line; }

    void set_line_index(int index) { m_line_index = index; }
    void set_index_into_line(int index) { m_index_into_line = index; }
    void set(int line_index, int index_into_line) {
        set_line_index(line_index);
        set_index_into_line(index_into_line);
    }

    int row_position(const Panel& panel) const;
    int col_position(const Panel& panel) const;

    bool at_document_start() const { return m_line_index == 0 && m_index_into_line == 0; }
    bool at_document_end() const;

private:
    Document& m_document;
    int m_line_index { 0 };
    int m_index_into_line { 0 };
};
};
