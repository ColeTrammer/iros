#pragma once

#include <app/object.h>
#include <liim/function.h>
#include <liim/string.h>
#include <liim/vector.h>

class Point;

namespace App {

class Window;

class ContextMenu : public Object {
    APP_OBJECT(ContextMenu)

public:
    ContextMenu(SharedPtr<Window> parent_window) : m_parent_window(parent_window) {}
    virtual ~ContextMenu() override;

    void add_menu_item(String name, Function<void()> hook);

    void show(Point p);
    void hide();
    bool visible() const;

private:
    Window& ensure_window(Point p);

    struct MenuItem {
        String name;
        Function<void()> on_click;
    };

    Vector<MenuItem> m_menu_items;
    SharedPtr<Window> m_window;
    SharedPtr<Window> m_parent_window;
};

}
