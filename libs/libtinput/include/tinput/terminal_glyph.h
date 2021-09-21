#pragma once

#include <liim/forward.h>
#include <liim/string.h>

namespace TInput {
class TerminalGlyph {
public:
    TerminalGlyph(String text, int width) : m_text(move(text)), m_width(width) {}

    const String& text() const { return m_text; }
    int width() const { return m_width; }

private:
    String m_text;
    int m_width { 0 };
};

class Glyphs {
public:
    void add(TerminalGlyph glyph) {
        m_glyphs.add(glyph);
        m_total_width += glyph.width();
    }

    auto begin() const { return m_glyphs.begin(); }
    auto end() const { return m_glyphs.end(); }

    int total_width() const { return m_total_width; }
    int size() const { return m_glyphs.size(); }

    const TerminalGlyph& operator[](int index) const { return m_glyphs[index]; }

private:
    Vector<TerminalGlyph> m_glyphs;
    int m_total_width { 0 };
};

Glyphs convert_to_glyphs(const StringView& view);
}
