#pragma once

#include <app/base/scroll_component.h>
#include <app/forward.h>
#include <eventloop/component.h>
#include <graphics/forward.h>
#include <graphics/point.h>
#include <graphics/rect.h>
#include <tinput/forward.h>
#include <tui/forward.h>

namespace TUI {
class ScrollComponent : public App::Base::ScrollComponentBridge {
    APP_BASE_SCROLL_COMPONENT_INTERFACE_FORWARD(base())

public:
    static constexpr int s_scrollbar_width = 1;

    explicit ScrollComponent(Panel& panel);
    virtual ~ScrollComponent() override;

    void attach_to_base(App::Base::ScrollComponent& base);

    virtual int scrollbar_width() const override { return s_scrollbar_width; }

    TInput::TerminalRenderer get_renderer();
    void draw_scrollbars();

private:
    void did_attach();

    App::Base::ScrollComponent& base() { return *m_base; }
    const App::Base::ScrollComponent& base() const { return *m_base; }

    Panel& panel() { return m_panel; }
    const Panel& panel() const { return m_panel; }

    Panel& m_panel;
    App::Base::ScrollComponent* m_base { nullptr };
};
}
