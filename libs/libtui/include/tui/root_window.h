#pragma once

#include <app/base/window.h>

namespace TUI {
class RootWindow final : public App::Base::Window {
    APP_OBJECT(RootWindow)

public:
    RootWindow() {}
    virtual void initialize() override;

protected:
    virtual void do_render() override;
};
}
