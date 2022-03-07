#pragma once

#include <app/widget.h>
#include <eventloop/event.h>
#include <liim/function.h>
#include <liim/string.h>

APP_EVENT(App, ClickEvent, Event, (), (), ())

namespace App {
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
