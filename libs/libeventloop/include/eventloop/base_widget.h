#pragma once

#include <eventloop/forward.h>
#include <eventloop/object.h>

namespace App {
class BaseWidget : public Object {
    APP_OBJECT(BaseWidget)

public:
    virtual ~BaseWidget() override;

    virtual void on_mouse_event(const MouseEvent&);
    virtual void on_key_event(const KeyEvent&);
    virtual void on_text_event(const TextEvent&) {}
    virtual void on_theme_change_event(const ThemeChangeEvent&) {}
    virtual void on_resize() {}
    virtual void on_focused() {}
    virtual void on_unfocused() {}
    virtual void on_leave() {}

    virtual void on_key_down(const KeyEvent&) {}
    virtual void on_key_up(const KeyEvent&) {}

    virtual void on_mouse_down(const MouseEvent&) {}
    virtual void on_mouse_double(const MouseEvent&);
    virtual void on_mouse_triple(const MouseEvent&);
    virtual void on_mouse_up(const MouseEvent&) {}
    virtual void on_mouse_move(const MouseEvent&) {}
    virtual void on_mouse_scroll(const MouseEvent&) {}

    virtual void on_event(const Event&);

    void set_key_bindings(UniquePtr<KeyBindings> bindings);
    bool handle_as_key_shortcut(const KeyEvent& event);

protected:
    BaseWidget();

private:
    UniquePtr<KeyBindings> m_key_bindings;
};
}
