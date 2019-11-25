#pragma once

#include <liim/vector.h>
#include <memory>

#include "window.h"

class WindowManager {
public:
    WindowManager(std::shared_ptr<PixelBuffer> pixels);
    ~WindowManager();

    Vector<std::shared_ptr<Window>>& windows() { return m_windows; }
    const Vector<std::shared_ptr<Window>>& windows() const { return m_windows; }

    void add_window(std::shared_ptr<Window> window);

    template<typename C>
    void for_each_window(C callback)
    {
        m_windows.for_each(callback);
    }

    void draw();

private:
    std::shared_ptr<PixelBuffer> m_pixels;
    Vector<std::shared_ptr<Window>> m_windows;
};