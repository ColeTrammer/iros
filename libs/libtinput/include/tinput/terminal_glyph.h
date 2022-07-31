#pragma once

#include <liim/forward.h>
#include <liim/string.h>

namespace TInput {
class TerminalGlyph {
public:
    TerminalGlyph(String text, size_t width) : m_text(move(text)), m_width(width) {}

    String& text() { return m_text; }
    const String& text() const { return m_text; }
    int width() const { return m_width; }

private:
    String m_text;
    size_t m_width { 0 };
};

class Glyphs {
public:
    void add(TerminalGlyph glyph) {
        m_glyphs.add(glyph);
        m_total_width += glyph.width();
    }

    auto begin() const { return m_glyphs.begin(); }
    auto end() const { return m_glyphs.end(); }

    TerminalGlyph& first() { return m_glyphs.first(); }
    TerminalGlyph& last() { return m_glyphs.last(); }

    int total_width() const { return m_total_width; }
    int size() const { return m_glyphs.size(); }
    bool empty() const { return m_glyphs.empty(); }

    const TerminalGlyph& operator[](int index) const { return m_glyphs[index]; }

private:
    Vector<TerminalGlyph> m_glyphs;
    int m_total_width { 0 };
};

Glyphs convert_to_glyphs(const StringView& view);
}
