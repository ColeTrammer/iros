#pragma once

#include <app/base/widget.h>
#include <graphics/rect.h>
#include <liim/forward.h>
#include <tinput/forward.h>
#include <tui/forward.h>

namespace TUI {
class Panel : public App::Base::Widget {
    APP_OBJECT(Panel)

public:
    Panel();
    virtual void initialize() override;
    virtual ~Panel() override;

    Panel* parent_panel();

protected:
    TInput::TerminalRenderer get_renderer();

private:
    virtual bool is_panel() const final { return true; }
};
}
