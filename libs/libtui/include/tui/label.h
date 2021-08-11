#pragma once

#include <graphics/text_align.h>
#include <liim/string.h>
#include <tinput/terminal_text_style.h>
#include <tui/panel.h>

namespace TUI {
class Label : public Panel {
    APP_OBJECT(Label)

public:
    explicit Label(String text) : m_text(move(text)) {}

    virtual void render() override;

    void set_text_align(TextAlign align);
    void set_terminal_text_style(const TInput::TerminalTextStyle& style);
    void set_shrink_to_fit(bool b);

private:
    String m_text;
    TextAlign m_text_align { TextAlign::CenterLeft };
    TInput::TerminalTextStyle m_text_style;
    bool m_shrink_to_fit { false };
};
}
