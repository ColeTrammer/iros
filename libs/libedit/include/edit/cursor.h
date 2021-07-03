#pragma once

#include <edit/forward.h>
#include <stddef.h>

namespace Edit {
class Cursor {
public:
    Cursor(Document& document, Panel& panel) : m_document(document), m_panel(panel) {}

    Line& referenced_line() const;
    char referenced_character() const;

    int line_index() const { return m_line_index; }
    int index_into_line() const { return m_index_into_line; }

    void set_line_index(int index) { m_line_index = index; }
    void set_index_into_line(int index) { m_index_into_line = index; }

    int row_position() const;
    int col_position() const;

private:
    Document& m_document;
    Panel& m_panel;
    int m_line_index { 0 };
    int m_index_into_line { 0 };
};
};
