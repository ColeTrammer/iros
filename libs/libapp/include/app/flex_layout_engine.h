#pragma once

#include <app/forward.h>
#include <app/layout_engine.h>
#include <graphics/forward.h>

namespace App {
class FlexLayoutEngine : public LayoutEngine {
public:
    enum class Direction { Horizontal, Vertical };

    FlexLayoutEngine(Widget& parent, Direction direction);
    virtual ~FlexLayoutEngine() override;

    virtual void layout() override;
    virtual void do_add(Widget& widget) override;
    virtual void do_remove(Widget& widget) override;

    void set_spacing(int spacing) { m_spacing = spacing; }
    int spacing() const { return m_spacing; }

private:
    int available_space() const;
    int visible_item_count() const;
    int auto_sized_item_count() const;
    bool auto_sized(const Widget& item) const;

    Direction m_direction;
    Vector<SharedPtr<Widget>> m_items;
    int m_spacing { 0 };
};

class HorizontalFlexLayoutEngine final : public FlexLayoutEngine {
public:
    HorizontalFlexLayoutEngine(Widget& widget) : FlexLayoutEngine(widget, FlexLayoutEngine::Direction::Horizontal) {}
};

class VerticalFlexLayoutEngine final : public FlexLayoutEngine {
public:
    VerticalFlexLayoutEngine(Widget& widget) : FlexLayoutEngine(widget, FlexLayoutEngine::Direction::Vertical) {}
};
}
