#pragma once

#include <liim/pointers.h>
#include <liim/vector.h>

#include "window.h"

class WindowManager {
public:
    WindowManager(int fb, SharedPtr<PixelBuffer> front_buffer, SharedPtr<PixelBuffer> back_buffer);
    ~WindowManager();

    Vector<SharedPtr<Window>>& windows() { return m_windows; }
    const Vector<SharedPtr<Window>>& windows() const { return m_windows; }

    void add_window(SharedPtr<Window> window);

    template<typename C>
    void for_each_window(C callback) {
        m_windows.for_each(callback);
    }

    void draw();

    Window* active_window() { return m_active_window; }
    const Window* active_window() const { return m_active_window; };

private:
    void swap_buffers();

    int m_fb;
    SharedPtr<PixelBuffer> m_front_buffer;
    SharedPtr<PixelBuffer> m_back_buffer;
    Vector<SharedPtr<Window>> m_windows;
    Window* m_active_window { nullptr };
};