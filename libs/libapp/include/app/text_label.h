#pragma once

#include <app/widget.h>
#include <graphics/text_align.h>
#include <liim/string.h>

namespace App {

class TextLabel : public Widget {
    APP_OBJECT(TextLabel)

public:
    TextLabel(String text) : m_text(move(text)) {}

    const String& text() const { return m_text; }
    void set_text(String text) { m_text = move(text); }

    TextAlign text_align() const { return m_text_align; }
    void set_text_align(TextAlign align) { m_text_align = align; }

protected:
    virtual void render() override;

private:
    String m_text;
    TextAlign m_text_align { TextAlign::CenterLeft };
};

}
