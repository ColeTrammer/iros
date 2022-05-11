#pragma once

#include <gui/widget.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace GUI {
class TabWidget : public Widget {
    APP_WIDGET(Widget, TabWidget)

public:
    TabWidget() {}
    virtual void did_attach() override;
    virtual void render() override;

    template<typename T, typename... Args>
    T& add_tab(String name, Args&&... args) {
        auto widget = create_widget_owned<T>(forward<Args>(args)...);
        if (active_tab() != -1 && m_tabs.size() != active_tab()) {
            widget->set_hidden(true);
        } else if (active_tab() == -1) {
            widget->set_hidden(false);
            m_active_tab = m_tabs.size();
        }
        widget->set_positioned_rect(tab_content_rect());

        auto rect = next_tab_rect(name);
        m_tabs.add({ move(name), rect, widget });
        return *widget;
    }

    void remove_tab(int index);

    int active_tab() const { return m_active_tab; }
    void set_active_tab(int index);

    const Rect& tab_content_rect() const { return m_tab_content_rect; }

protected:
    // virtual void did_remove_child(SharedPtr<Object>) override;

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
