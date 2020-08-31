#pragma once

#include <app/timer.h>
#include <graphics/rect.h>
#include <liim/pointers.h>
#include <liim/vector.h>
#include <kernel/hal/input.h>

#include "window.h"

class Renderer;

class Taskbar {
public:
    Taskbar(int display_width, int display_height);

    void render(Renderer&);

    void notify_window_added(SharedPtr<Window> window);
    void notify_window_removed(Window& window);
    bool notify_mouse_pressed(int mouse_x, int mouse_y, mouse_button_state left, mouse_button_state right);

private:
    struct TaskbarItem {
        Rect rect;
        SharedPtr<Window> window;
    };

    Vector<TaskbarItem> m_items;
    SharedPtr<App::Timer> m_time_timer;
    int m_display_width;
    int m_display_height;
};
