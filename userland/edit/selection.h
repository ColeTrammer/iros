#pragma once

#include <liim/string.h>

class Document;

class Selection {
public:
    Selection(Document& document) : m_document(document) {}
    ~Selection() {}

    Document& document() { return m_document; }
    const Document& document() const { return m_document; }

    void clear() { m_start_line = m_start_index = m_end_line = m_end_index = 0; }
    bool empty() const { return m_start_line == m_end_line && m_start_index == m_end_index; }

    void begin(int line, int index) {
        m_start_line = m_end_line = line;
        m_start_index = m_end_index = index;
    }

    void set_end_line(int line) { m_end_line = line; }
    void set_end_index(int index) { m_end_index = index; }

private:
    Document& m_document;
    int m_start_line { 0 };
    int m_start_index { 0 };
    int m_end_line { 0 };
    int m_end_index { 0 };
};