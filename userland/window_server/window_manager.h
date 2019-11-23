#pragma once

#include <liim/vector.h>
#include <liim/pointers.h>

#include "window.h"

class WindowManager {
public:
    WindowManager();
    ~WindowManager();

    Vector<SharedPtr<Window>>& windows() { return m_windows; }
    const Vector<SharedPtr<Window>>& windows() const { return m_windows; }

    void add_window(SharedPtr<Window> window);

    template<typename C>
    void for_each_window(C callback)
    {
        windows().for_each(callback);
    }

private:
    Vector<SharedPtr<Window>> m_windows;
};