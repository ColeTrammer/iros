#pragma once

#include <graphics/rect_set.h>
#include <liim/function.h>
#include <liim/pointers.h>
#include <liim/vector.h>
#include <kernel/hal/input.h>

#include "taskbar.h"
#include "window.h"

class Point;

class WindowManager {
public:
    static WindowManager& the();

    enum class ResizeMode {
        Invalid,
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
        Left,
    };

    WindowManager(int fb, SharedPtr<PixelBuffer> front_buffer, SharedPtr<PixelBuffer> back_buffer);
    ~WindowManager();

    Vector<SharedPtr<Window>>& windows() { return m_windows; }
    const Vector<SharedPtr<Window>>& windows() const { return m_windows; }

    void add_window(SharedPtr<Window> window);

    template<typename C>
    void for_each_window(C callback) {
        m_windows.for_each(callback);
    }

    void remove_window(wid_t wid);
    void remove_windows_of_client(int client_id);

    void draw();

    Window* active_window() { return m_active_window.get(); }
    const Window* active_window() const { return m_active_window.get(); };

    void notify_mouse_moved(int dx, int dy, bool absolute);
    void notify_mouse_pressed(mouse_button_state left, mouse_button_state right);

    void set_active_window(SharedPtr<Window> window);
    void move_to_front_and_make_active(SharedPtr<Window> window);

    int find_window_intersecting_point(Point p);
    int find_window_intersecting_rect(const Rect& r);

    Point mouse_position_relative_to_window(const Window& window) const;

    void invalidate_rect(const Rect& rect) { m_dirty_rects.add(rect); }

    Function<void(Window&)> on_window_close_button_pressed;
    Function<void(Window&)> on_window_resize_start;

    bool should_send_mouse_events(const Window& window) const {
        return &window != m_window_to_move.get() && &window != m_window_to_resize.get();
    }

private:
    void swap_buffers();
    void set_mouse_coordinates(int x, int y);

    int m_fb { -1 };
    int m_mouse_x { 0 };
    int m_mouse_y { 0 };
    RectSet m_dirty_rects;
    SharedPtr<PixelBuffer> m_front_buffer;
    SharedPtr<PixelBuffer> m_back_buffer;
    Vector<SharedPtr<Window>> m_windows;
    Taskbar m_taskbar;
    SharedPtr<Window> m_active_window;

    SharedPtr<Window> m_window_to_resize;
    ResizeMode m_window_resize_mode { ResizeMode::Invalid };

    SharedPtr<Window> m_window_to_move;
    Point m_window_move_initial_location;
    Point m_window_move_origin;
};
