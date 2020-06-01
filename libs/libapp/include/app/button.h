#pragma once

#include <app/widget.h>
#include <liim/function.h>
#include <liim/string.h>

namespace App {

class Button : public Widget {
    APP_OBJECT(Button)

public:
    Button(String label) : m_label(move(label)) {}

    void set_label(String label) { m_label = move(label); }
    const String& label() const { return m_label; }

    Function<void()> on_click;

private:
    virtual void render() override;
    virtual void on_mouse_event(MouseEvent& mouse_event) override;

    String m_label;
    bool m_did_mousedown { false };
};

}
