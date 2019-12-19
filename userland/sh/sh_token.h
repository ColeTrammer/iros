#pragma once

#include <liim/string_view.h>
#include <stddef.h>

class ShValue {
public:
    ShValue(const StringView& text, size_t line, size_t position) : m_text(text), m_line(line), m_position(position) {}

    const StringView& text() const { return m_text; }
    size_t line() const { return m_line; }
    size_t position() const { return m_position; }

private:
    StringView m_text;
    size_t m_line;
    size_t m_position;
};