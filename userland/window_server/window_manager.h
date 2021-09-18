#pragma once

#include <eventloop/event.h>
#include <eventloop/file_watcher.h>
#include <graphics/bitmap.h>
#include <graphics/palette.h>
#include <graphics/rect_set.h>
#include <liim/function.h>
#include <liim/hash_map.h>
#include <liim/pointers.h>
#include <liim/vector.h>
#include <kernel/hal/input.h>

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

    WindowManager(int fb, SharedPtr<Bitmap> front_buffer, SharedPtr<Bitmap> back_buffer);
    ~WindowManager();

    void add_window(SharedPtr<Window> window);
    void set_window_visibility(SharedPtr<Window> window, bool visible);

    void remove_window(SharedPtr<Window> window);
    void remove_windows_of_client(SharedPtr<IPC::Endpoint> client);

    void draw();

    bool load_palette(const String& path);

    Window* active_window() { return m_active_window.get(); }
    const Window* active_window() const { return m_active_window.get(); };

    void notify_mouse_moved(const App::MouseEvent& event);
    void notify_mouse_pressed(const App::MouseEvent& event);
    void notify_mouse_input(const App::MouseEvent& event);

    void set_active_window(SharedPtr<Window> window);
    void move_to_front_and_make_active(SharedPtr<Window> window);

    SharedPtr<Window> find_window_intersecting_point(Point p);
    SharedPtr<Window> find_window_intersecting_rect(const Rect& r);

    Point mouse_position_relative_to_window(const Window& window) const;

    SharedPtr<Window> find_by_wid(wid_t wid);

    void invalidate_rect(const Rect& rect);

    bool should_send_mouse_events(const Window& window) const {
        return &window != m_window_to_move.get() && &window != m_window_to_resize.get();
    }

    bool drawing() const { return m_drawing; }

    bool window_exactly_at(Point p) const;

    Rect screen_rect() const { return { 0, 0, m_front_buffer->width(), m_front_buffer->height() }; }

    SharedPtr<Palette> palette() const { return m_palette; }

    Function<void(SharedPtr<Window>)> on_window_close_button_pressed;
    Function<void(SharedPtr<Window>)> on_window_resize_start;
    Function<void(SharedPtr<Window>, bool active)> on_window_state_change;
    Function<void(wid_t)> on_window_removed;
    Function<void()> on_rect_invaliadted;
    Function<void()> on_palette_changed;

private:
    void cleanup_active_window_state(SharedPtr<Window> window);

    void swap_buffers();
    void set_mouse_coordinates(int x, int y);

    int m_fb { -1 };
    int m_mouse_x { 0 };
    int m_mouse_y { 0 };
    RectSet m_dirty_rects;
    SharedPtr<Bitmap> m_front_buffer;
    SharedPtr<Bitmap> m_back_buffer;
    SharedPtr<Palette> m_palette;
    Vector<SharedPtr<Window>> m_window_stack;
    HashMap<wid_t, SharedPtr<Window>> m_window_map;
    SharedPtr<Window> m_active_window;
    SharedPtr<App::FileWatcher> m_watcher;
    String m_palette_path;

    SharedPtr<Window> m_window_to_resize;
    ResizeMode m_window_resize_mode { ResizeMode::Invalid };

    SharedPtr<Window> m_window_to_move;
    Point m_window_move_initial_location;
    Point m_window_move_origin;

    SharedPtr<Bitmap> m_desktop_background;

    bool m_drawing { false };
};
