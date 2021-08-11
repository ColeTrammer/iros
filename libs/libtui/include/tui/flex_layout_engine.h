#pragma once

#include <tui/layout_engine.h>

namespace TUI {
class FlexLayoutEngine final : public LayoutEngine {
public:
    enum class Direction { Horizontal, Vertical };

    FlexLayoutEngine(Panel& parent, Direction direction);
    virtual ~FlexLayoutEngine() override;

    virtual void layout() override;
    virtual void do_add(Panel& panel) override;

    void set_padding(int padding) { m_padding = padding; }
    Rect parent_rect() const;

private:
    int auto_sized_item_count() const;
    int available_space() const;
    bool auto_sized(const Panel& item) const;

    Direction m_direction;
    Vector<SharedPtr<Panel>> m_items;
    int m_padding { 0 };
};
}
