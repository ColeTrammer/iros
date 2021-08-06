#pragma once

#include <liim/string.h>
#include <liim/vector.h>

namespace Edit {
class Suggestion {
public:
    Suggestion(String content, size_t offset) : m_content(move(content)), m_offset(offset) {}

    size_t offset() const { return m_offset; }
    const String& content() const { return m_content; }

private:
    String m_content;
    size_t m_offset;
};

class Suggestions {
public:
    auto begin() const { return m_suggestions.begin(); }
    auto end() const { return m_suggestions.end(); }

    int size() const { return m_suggestions.size(); }
    bool empty() const { return m_suggestions.empty(); }

    const Suggestion& first() const { return m_suggestions.first(); }
    const Suggestion& last() const { return m_suggestions.last(); }

    const Suggestion& operator[](int index) const { return m_suggestions[index]; }

    void clear() { m_suggestions.clear(); }

    const Vector<Suggestion> suggestions() const { return m_suggestions; }
    void set_suggestions(Vector<Suggestion> suggestions) { m_suggestions = move(suggestions); }

private:
    Vector<Suggestion> m_suggestions;
};
}
