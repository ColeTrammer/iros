#include "window_manager.h"

WindowManager::WindowManager()
{
}

WindowManager::~WindowManager()
{
}

void WindowManager::add_window(std::shared_ptr<Window> window)
{
    windows().push_back(window);
}