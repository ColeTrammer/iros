#pragma once

#include <liim/string.h>
#include <liim/utilities.h>

class Document;

class Selection {
public:
    Selection() {}

    void clear() { m_start_line = m_start_index = m_end_line = m_end_index = 0; }
    bool empty() const { return m_start_line == m_end_line && m_start_index == m_end_index; }

    void begin(int line, int index) {
        m_start_line = m_end_line = line;
        m_start_index = m_end_index = index;
    }

    void set_end_line(int line) { m_end_line = line; }
    void set_end_index(int index) { m_end_index = index; }

    int upper_line() const { return LIIM::min(m_start_line, m_end_line); }
    int upper_index() const {
        if (m_start_line == m_end_line) {
            return LIIM::min(m_start_index, m_end_index);
        } else if (m_start_line < m_end_line) {
            return m_start_index;
        } else {
            return m_end_index;
        }
    }

    int lower_line() const { return LIIM::max(m_start_line, m_end_line); }
    int lower_index() const {
        if (m_start_line == m_end_line) {
            return LIIM::max(m_start_index, m_end_index);
        } else if (m_start_line < m_end_line) {
            return m_end_index;
        } else {
            return m_start_index;
        }
    }

private:
    int m_start_line { 0 };
    int m_start_index { 0 };
    int m_end_line { 0 };
    int m_end_index { 0 };
};