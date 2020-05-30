#pragma once

#include <app/widget.h>
#include <liim/string.h>

namespace App {

class TextLabel : public Widget {
    APP_OBJECT(TextLabel)

public:
    TextLabel(String text) : m_text(move(text)) {}

    const String& text() const { return m_text; }
    void set_text(String text) { m_text = move(text); }

protected:
    virtual void render() override;

private:
    String m_text;
};

}
