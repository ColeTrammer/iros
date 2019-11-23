#include "window_manager.h"

WindowManager::WindowManager()
{
}

WindowManager::~WindowManager()
{
}

void WindowManager::add_window(SharedPtr<Window> window)
{
    windows().add(window);
}