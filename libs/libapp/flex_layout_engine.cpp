#include <app/base/application.h>
#include <app/flex_layout_engine.h>

namespace App {
template<typename T>
static constexpr int size_in_layout_direction(FlexLayoutEngine::Direction direction, const T& size) {
    return direction == FlexLayoutEngine::Direction::Horizontal ? size.width() : size.height();
}

FlexLayoutEngine::FlexLayoutEngine(Base::Widget& parent, Direction direction) : LayoutEngine(parent), m_direction(direction) {
    set_spacing(App::Base::Application::the().default_spacing());
}

FlexLayoutEngine::~FlexLayoutEngine() {}

void FlexLayoutEngine::layout() {
    if (m_items.empty() || parent_rect().empty()) {
        return;
    }

    auto ignore_layout_constraints = false;

    auto flex_count = auto_sized_item_count();
    auto space = available_space();
    if (space == 0) {
        flex_count = visible_item_count();
        space = size_in_layout_direction(m_direction, parent_rect());
        ignore_layout_constraints = true;
    }

    auto distributed_size = flex_count != 0 ? space / flex_count : 0;

    auto width = parent_rect().width();
    auto height = parent_rect().height();
    if (m_direction == Direction::Horizontal) {
        width = distributed_size;
    } else {
        height = distributed_size;
    }

    auto x = parent_rect().x();
    auto y = parent_rect().y();
    for (auto& item : m_items) {
        if (item->hidden()) {
            continue;
        }

        auto width_to_use = width;
        auto height_to_use = height;
        if (!ignore_layout_constraints && !auto_sized(*item)) {
            auto& constraint = item->layout_constraint();
            if (m_direction == Direction::Horizontal) {
                width_to_use = constraint.width();
            } else {
                height_to_use = constraint.height();
            }
        }

        item->set_positioned_rect({ x, y, width_to_use, height_to_use });

        if (m_direction == Direction::Horizontal) {
            x += width_to_use + spacing();
        } else {
            y += height_to_use + spacing();
        }
    }
}

void FlexLayoutEngine::do_add(Base::Widget& widget) {
    m_items.add(widget.shared_from_this());
}

int FlexLayoutEngine::available_space() const {
    int fixed_space = max(0, (visible_item_count() - 1) * spacing());
    for (auto& item : m_items) {
        if (!auto_sized(*item)) {
            fixed_space += size_in_layout_direction(m_direction, item->layout_constraint());
        }
    }
    auto max_space = size_in_layout_direction(m_direction, parent_rect());
    return max(0, max_space - fixed_space);
}

int FlexLayoutEngine::visible_item_count() const {
    int count = 0;
    for (auto& item : m_items) {
        if (!item->hidden()) {
            count++;
        }
    }
    return count;
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

bool FlexLayoutEngine::auto_sized(const Base::Widget& item) const {
    return !item.hidden() && size_in_layout_direction(m_direction, item.layout_constraint()) == LayoutConstraint::AutoSize;
}
}
