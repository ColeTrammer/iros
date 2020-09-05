#pragma once

#include <app/widget.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace App {

class TabWidget : public Widget {
    APP_OBJECT(TabWidget)

public:
    virtual void on_mouse_event(MouseEvent&) override;
    virtual void on_focused() override;
    virtual void on_resize() override;
    virtual void render() override;

    template<typename T, typename... Args>
    T& add_tab(String name, Args... args) {
        auto ret = T::create(shared_from_this(), forward<Args>(args)...);
        if (active_tab() != -1 && m_tabs.size() != active_tab()) {
            ret->set_hidden(true);
        } else if (active_tab() == -1) {
            ret->set_hidden(false);
            m_active_tab = m_tabs.size();
        }
        ret->set_rect(tab_content_rect());

        auto rect = next_tab_rect(name);
        m_tabs.add({ move(name), rect, ret });
        return *ret;
    }

    void remove_tab(int index);

    int active_tab() const { return m_active_tab; }
    void set_active_tab(int index);

    const Rect& tab_content_rect() const { return m_tab_content_rect; }

protected:
    virtual void did_remove_child(SharedPtr<Object>) override;

    Rect next_tab_rect(const String& name) const;

private:
    struct Tab {
        String name;
        Rect rect;
        SharedPtr<Widget> widget;
    };

    Vector<Tab> m_tabs;
    Rect m_tab_content_rect;
    int m_active_tab { -1 };
};

}
