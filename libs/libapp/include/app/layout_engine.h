#pragma once

#include <app/base/widget.h>
#include <app/forward.h>

namespace App {
struct Margins {
    int top { 0 };
    int right { 0 };
    int bottom { 0 };
    int left { 0 };
};

class LayoutEngine {
public:
    explicit LayoutEngine(Base::Widget& parent);
    virtual ~LayoutEngine() {}

    virtual void layout() = 0;
    virtual void do_add(Base::Widget& child) = 0;

    void schedule_layout();

    void set_margins(const Margins& m) { m_margins = m; }
    const Margins& margins() const { return m_margins; }

    template<typename PanelType, typename... Args>
    PanelType& add(Args&&... args) {
        auto panel = PanelType::create(parent().shared_from_this(), forward<Args>(args)...);
        do_add(*panel);
        schedule_layout();
        return *panel;
    }

protected:
    Rect parent_rect() const;

    Base::Widget& parent() { return m_parent; }
    const Base::Widget& parent() const { return m_parent; }

private:
    Base::Widget& m_parent;
    Margins m_margins;
    bool m_layout_scheduled { false };
};
}
