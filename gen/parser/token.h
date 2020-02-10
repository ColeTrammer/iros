#pragma once

#include <liim/string_view.h>

template<typename Type>
struct Token {
public:
    Token(Type type, StringView text) : m_type(type), m_text(text) {}
    Token(const Token& other) : m_type(other.type()), m_text(other.text()) {}

    Type type() const { return m_type; };
    void set_type(Type type) { m_type = type; }

    const StringView& text() const { return m_text; }
    StringView& text() { return m_text; }

private:
    Type m_type;
    StringView m_text;
};