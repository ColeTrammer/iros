#pragma once

#include <app/window.h>

namespace TUI {
class Window final : public App::Window {
    APP_OBJECT(Window)

public:
    Window() {}
    virtual void initialize() override;

protected:
    virtual void do_render() override;
};
}
