#pragma once

#include <tui/forward.h>
#include <tui/panel.h>

namespace TUI {
class LayoutEngine {
public:
    explicit LayoutEngine(Panel& parent) : m_parent(parent) {}
    virtual ~LayoutEngine() {}

    virtual void layout() = 0;
    virtual void do_add(Panel& child) = 0;

    template<typename PanelType, typename... Args>
    PanelType& add(Args&&... args) {
        auto panel = PanelType::create(parent().shared_from_this(), forward<Args>(args)...);
        do_add(*panel);
        return *panel;
    }

protected:
    Panel& parent() { return m_parent; }
    const Panel& parent() const { return m_parent; }

private:
    Panel& m_parent;
};
}
