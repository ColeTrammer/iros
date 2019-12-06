#pragma once

#include <liim/vector.h>
#include <memory>

#include "window.h"

class WindowManager {
public:
    WindowManager(int fb, std::shared_ptr<PixelBuffer> front_buffer, std::shared_ptr<PixelBuffer> back_buffer);
    ~WindowManager();

    Vector<std::shared_ptr<Window>>& windows() { return m_windows; }
    const Vector<std::shared_ptr<Window>>& windows() const { return m_windows; }

    void add_window(std::shared_ptr<Window> window);

    template<typename C> void for_each_window(C callback) { m_windows.for_each(callback); }

    void draw();

private:
    void swap_buffers();

    int m_fb;
    std::shared_ptr<PixelBuffer> m_front_buffer;
    std::shared_ptr<PixelBuffer> m_back_buffer;
    Vector<std::shared_ptr<Window>> m_windows;
};