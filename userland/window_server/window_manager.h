#pragma once

#include <algorithm>
#include <liim/vector.h>
#include <memory>
#include <vector>

#include "window.h"

class WindowManager {
public:
    WindowManager();
    ~WindowManager();

    std::vector<std::shared_ptr<Window>>& windows() { return m_windows; }
    const std::vector<std::shared_ptr<Window>>& windows() const { return m_windows; }

    void add_window(std::shared_ptr<Window> window);

    template<typename C>
    void for_each_window(C callback)
    {
        std::for_each(m_windows.begin(), m_windows.end(), callback);
    }

private:
    std::vector<std::shared_ptr<Window>> m_windows;
};