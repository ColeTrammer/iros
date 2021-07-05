#pragma once

#include <assert.h>
#include <edit/forward.h>
#include <edit/text_index.h>
#include <stddef.h>

namespace Edit {
class Cursor {
public:
    Cursor(Document& document) : m_document(document) {}
    Cursor(const Cursor& other) = default;

    Cursor& operator=(const Cursor& other) {
        assert(&this->m_document == &other.m_document);
        this->m_index = other.m_index;
        return *this;
    }

    Line& referenced_line() const;
    char referenced_character() const;

    int line_index() const { return m_index.line_index(); }
    int index_into_line() const { return m_index.index_into_line(); }
    const TextIndex& index() const { return m_index; }

    void set_line_index(int line_index) { set({ line_index, index_into_line() }); }
    void set_index_into_line(int index_into_line) { set({ line_index(), index_into_line }); }
    void set(const TextIndex& index) { m_index = index; }

    Position relative_position(const Panel& panel) const;
    Position absolute_position(const Panel& panel) const;

    bool at_document_start() const { return m_index == TextIndex { 0, 0 }; }
    bool at_document_end() const;

private:
    Document& m_document;
    TextIndex m_index;
};
};
