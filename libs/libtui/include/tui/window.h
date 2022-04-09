#pragma once

#include <app/window.h>

namespace TUI {
class Window final : public App::Window {
    APP_OBJECT(Window)

public:
    Window() {}
    virtual void initialize() override;

    Window* parent_window();

    const Point& position() const { return m_position; }

    virtual void schedule_render() override;
    virtual void do_render() override;

private:
    void render_subwindows();

    Point m_position;
};
}
