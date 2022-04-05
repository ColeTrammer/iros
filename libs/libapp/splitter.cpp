#include <app/layout_engine.h>
#include <app/splitter.h>

namespace App::Detail {
SplitterLayoutEngine::SplitterLayoutEngine(Widget& parent) : LayoutEngine(parent) {}

SplitterLayoutEngine::~SplitterLayoutEngine() {}

void SplitterLayoutEngine::layout() {
    compute_layout();

    for (auto& item : m_items) {
        if (!item.widget->hidden()) {
            item.widget->set_positioned_rect(item.relative_rect.translated(parent().positioned_rect().top_left()));
        }
    }
}

void SplitterLayoutEngine::do_add(Widget& widget) {
    if (!widget.hidden()) {
        for (auto& item : m_items) {
            item.expected_fraction *= static_cast<double>(item_count()) / static_cast<double>(item_count() + 1);
        }

        double expected_fraction = 1.0 / (item_count() + 1);
        m_items.add({ expected_fraction, {}, widget.shared_from_this() });
    } else {
        m_items.add({ 0.0, {}, widget.shared_from_this() });
    }

    widget.on<HideEvent>(parent(), [this, &widget](auto&) {
        if (auto index = find_index_of_item(widget)) {
            for (auto& item : m_items) {
                item.expected_fraction *= static_cast<double>(item_count() + 1) / static_cast<double>(item_count());
            }

            m_items[*index].expected_fraction = 0.0;
        }
    });

    widget.on<ShowEvent>(parent(), [this, &widget](auto&) {
        if (auto index = find_index_of_item(widget)) {
            for (auto& item : m_items) {
                item.expected_fraction *= static_cast<double>(item_count() - 1) / static_cast<double>(item_count());
            }

            // FIXME: remember potentially old state.
            m_items[*index].expected_fraction = 1.0 / item_count();
        }
    });
}

void SplitterLayoutEngine::do_remove(Widget& widget) {
    if (m_items.size() == 1) {
        m_items.clear();
        return;
    }

    auto found_index = find_index_of_item(widget);
    assert(found_index);
    m_items.remove(*found_index);

    for (auto& item : m_items) {
        item.expected_fraction *= static_cast<double>(item_count() + 1) / static_cast<double>(item_count());
    }
}

Maybe<HoldStart> SplitterLayoutEngine::compute_hold_start(const Point&) const {
    return {};
}

void SplitterLayoutEngine::adjust_size_and_position(const HoldStart&, const Point&) {}

Maybe<int> SplitterLayoutEngine::find_index_of_item(Widget& widget) const {
    Maybe<int> found_index;
    for (int i = 0; i < m_items.size(); i++) {
        if (m_items[i].widget.get() == &widget) {
            return i;
        }
    }
    return {};
}

int SplitterLayoutEngine::rect_size_in_layout_direction(const Rect& rect) const {
    if (direction() == Direction::Horizontal) {
        return rect.width();
    }
    return rect.height();
}

int SplitterLayoutEngine::rect_size_against_layout_direction(const Rect& rect) const {
    if (direction() == Direction::Horizontal) {
        return rect.height();
    }
    return rect.width();
}

int SplitterLayoutEngine::available_space_in_layout_direction() const {
    return rect_size_in_layout_direction(parent().sized_rect());
}

int SplitterLayoutEngine::available_space_against_layout_direction() const {
    return rect_size_against_layout_direction(parent().sized_rect());
}

int SplitterLayoutEngine::item_count() const {
    int count = 0;
    for (auto& item : m_items) {
        if (!item.widget->hidden()) {
            count++;
        }
    }
    return count;
}

void SplitterLayoutEngine::compute_layout() {
    if (item_count() == 0) {
        return;
    }

    auto gutters = item_count() - 1;
    auto space_available = max(0, available_space_in_layout_direction() - gutters * gutter_width());

    int space_leftover = space_available;
    Point offset;
    for (auto& item : m_items) {
        if (item.widget->hidden()) {
            continue;
        }

        auto length = static_cast<int>(space_available * item.expected_fraction);
        auto width = direction() == Direction::Horizontal ? length : available_space_against_layout_direction();
        auto height = direction() == Direction::Vertical ? length : available_space_against_layout_direction();

        // FIXME: potenially consider this layout's margins.
        // FIXME: allow "fixed" sized items.
        item.relative_rect = { offset.x(), offset.y(), width, height };

        if (direction() == Direction::Horizontal) {
            offset.set_x(offset.x() + length + gutter_width());
        } else {
            offset.set_y(offset.y() + length + gutter_width());
        }
        space_leftover -= length;
    }

    // FIXME: maybe not all left over space should go to the last item
    if (direction() == Direction::Horizontal) {
        m_items.last().relative_rect = m_items.last().relative_rect.expanded(space_leftover, 0);
    } else {
        m_items.last().relative_rect = m_items.last().relative_rect.expanded(0, space_leftover);
    }
}
}

namespace App {
Splitter::Splitter(SharedPtr<WidgetBridge> widget_bridge, SharedPtr<SplitterBridge> splitter_bridge)
    : Widget(move(widget_bridge)), m_bridge(move(splitter_bridge)) {}

void Splitter::initialize() {
    set_accepts_focus(true);

    auto& layout = set_layout_engine<Detail::SplitterLayoutEngine>();
    layout.set_direction(m_direction);
    layout.set_gutter_width(gutter_width());

    on<MouseDownEvent>([this, &layout](const MouseDownEvent& event) {
        auto mouse_position = Point { event.x(), event.y() };
        if (event.left_button()) {
            m_hold_start = layout.compute_hold_start(mouse_position);
            return m_hold_start.has_value();
        }
        return false;
    });

    on<MouseMoveEvent>([this, &layout](const MouseMoveEvent& event) {
        auto mouse_position = Point { event.x(), event.y() };
        if (m_hold_start && (event.buttons_down() & MouseButton::Left)) {
            layout.adjust_size_and_position(*m_hold_start, mouse_position);
            return true;
        }
        return false;
    });

    on<MouseUpEvent>([this](const MouseUpEvent& event) {
        if (event.left_button()) {
            m_hold_start.reset();
            return true;
        }
        return false;
    });

    Widget::initialize();
}

Splitter::~Splitter() {}

void Splitter::set_direction(App::Direction direction) {
    layout().set_direction(direction);
}

const Detail::SplitterLayoutEngine& Splitter::layout() const {
    return static_cast<const Detail::SplitterLayoutEngine&>(*layout_engine());
}
}
