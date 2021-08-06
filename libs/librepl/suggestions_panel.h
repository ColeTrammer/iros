#pragma once

#include <edit/suggestions.h>
#include <tui/panel.h>

namespace Repl {
class ReplDisplay;

class SuggestionsPanel final : public TUI::Panel {
    APP_OBJECT(SuggestionsPanel)

public:
    SuggestionsPanel(ReplDisplay& display);
    virtual ~SuggestionsPanel() override;

    virtual void render() override;
    virtual void on_key_event(const App::KeyEvent& event) override;

    void set_suggestions(Vector<Edit::Suggestion> suggestions);

private:
    ReplDisplay& m_display;
    Vector<Edit::Suggestion> m_suggestions;
    int m_suggestion_index { 0 };
    int m_suggestion_offset { 0 };
};
}
