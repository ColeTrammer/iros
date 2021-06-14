#pragma once

#include <liim/string.h>
#include <liim/vector.h>

class Suggestions {
public:
    Suggestions() {}

    Suggestions(size_t suggestion_offset, Vector<String>&& suggestion_list)
        : m_suggestion_list(suggestion_list), m_suggestion_offset(suggestion_offset) {}

    Vector<String>& suggestion_list() { return m_suggestion_list; }
    const Vector<String>& suggestion_list() const { return m_suggestion_list; }

    int suggestion_count() const { return m_suggestion_list.size(); }
    size_t suggestion_offset() const { return m_suggestion_offset; }

private:
    Vector<String> m_suggestion_list;
    size_t m_suggestion_offset { 0 };
};
