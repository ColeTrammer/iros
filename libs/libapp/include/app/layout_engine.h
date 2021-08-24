#pragma once

#include <app/base/widget.h>
#include <app/forward.h>

namespace App {
class LayoutEngine {
public:
    explicit LayoutEngine(Base::Widget& parent) : m_parent(parent) {}
    virtual ~LayoutEngine() {}

    virtual void layout() = 0;
    virtual void do_add(Base::Widget& child) = 0;

    template<typename PanelType, typename... Args>
    PanelType& add(Args&&... args) {
        auto panel = PanelType::create(parent().shared_from_this(), forward<Args>(args)...);
        do_add(*panel);
        return *panel;
    }

protected:
    Base::Widget& parent() { return m_parent; }
    const Base::Widget& parent() const { return m_parent; }

private:
    Base::Widget& m_parent;
};
}
