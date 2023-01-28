#pragma once

#include <eventloop/event.h>
#include <gui/widget.h>
#include <liim/function.h>
#include <liim/string.h>

APP_EVENT(GUI, ClickEvent, App::Event, (), (), ())

namespace GUI {
class Button : public Widget {
    APP_WIDGET_EMITS(Widget, Button, (ClickEvent))

public:
    explicit Button(String label);
    virtual void did_attach() override;

    void set_label(String label) { m_label = move(label); }
    const String& label() const { return m_label; }

    virtual void render() override;

    String m_label;
    bool m_did_mousedown { false };
};
}
