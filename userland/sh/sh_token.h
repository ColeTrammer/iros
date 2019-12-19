#pragma once

#include <liim/maybe.h>
#include <liim/string.h>
#include <liim/string_view.h>
#include <liim/vector.h>
#include <stddef.h>
#include <stdio.h>

class ShValue {
public:
    ShValue() {}
    ShValue(const StringView& text, size_t line, size_t position) : m_line(line), m_position(position) { m_words.add(text); }
    ShValue(const ShValue& other)
        : m_line(other.line()), m_position(other.position()), m_words(other.words()), m_pipelines(other.m_pipelines) {}

    Maybe<StringView> text() const { return m_words.size() > 0 ? m_words.get(0) : Maybe<StringView> {}; }
    size_t line() const { return m_line; }
    size_t position() const { return m_position; }

    Vector<StringView>& words() { return m_words; }
    const Vector<StringView>& words() const { return m_words; }

    using Command = Vector<StringView>;

    Vector<Command>& pipelines() { return m_pipelines.value(); }
    const Vector<Command>& pipelines() const { return m_pipelines.value(); }

    bool has_pipelines() const { return m_pipelines.has_value(); }

    void create_pipeline(Command command) {
        m_pipelines = Vector<Command>();
        pipelines().add(command);
    }

private:
    size_t m_line { 0 };
    size_t m_position { 0 };
    Vector<StringView> m_words;
    Maybe<Vector<Command>> m_pipelines;
};