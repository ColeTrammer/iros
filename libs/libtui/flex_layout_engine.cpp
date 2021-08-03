#include <tui/flex_layout_engine.h>

namespace TUI {
template<typename T>
static constexpr int size_in_layout_direction(FlexLayoutEngine::Direction direction, const T& size) {
    return direction == FlexLayoutEngine::Direction::Horizontal ? size.width() : size.height();
}

FlexLayoutEngine::FlexLayoutEngine(Panel& parent, Direction direction) : LayoutEngine(parent), m_direction(direction) {}

FlexLayoutEngine::~FlexLayoutEngine() {}

int FlexLayoutEngine::available_space() const {
    int fixed_space = 0;
    for (auto& item : m_items) {
        if (!auto_sized(*item)) {
            fixed_space += size_in_layout_direction(m_direction, item->layout_constraint());
        }
    }
    auto max_space = size_in_layout_direction(m_direction, parent().positioned_rect());
    return max(0, max_space - fixed_space);
}

int FlexLayoutEngine::auto_sized_item_count() const {
    int count = 0;
    for (auto& item : m_items) {
        if (auto_sized(*item)) {
            count++;
        }
    }
    return count;
}

bool FlexLayoutEngine::auto_sized(const Panel& item) const {
    return size_in_layout_direction(m_direction, item.layout_constraint()) == LayoutConstraint::AutoSize;
}

void FlexLayoutEngine::layout() {
    if (m_items.empty()) {
        return;
    }

    auto flex_count = auto_sized_item_count();
    auto space = available_space();
    auto distributed_size = space / flex_count;

    auto width = parent().positioned_rect().width();
    auto height = parent().positioned_rect().height();
    if (m_direction == Direction::Horizontal) {
        width = distributed_size;
    } else {
        height = distributed_size;
    }

    auto x = parent().positioned_rect().left();
    auto y = parent().positioned_rect().top();
    for (auto& item : m_items) {
        auto width_to_use = width;
        auto height_to_use = height;
        if (!auto_sized(*item)) {
            auto& constraint = item->layout_constraint();
            if (m_direction == Direction::Horizontal) {
                width_to_use = constraint.width();
            } else {
                height_to_use = constraint.height();
            }
        }
        item->set_positioned_rect({ x, y, width, height });

        if (m_direction == Direction::Horizontal) {
            x += width_to_use;
        } else {
            y += height_to_use;
        }
    }
}

void FlexLayoutEngine::do_add(Panel& panel) {
    m_items.add(panel.shared_from_this());
}
}
