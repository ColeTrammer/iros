#include <app/layout_engine.h>
#include <app/splitter.h>

namespace App::Detail {
SplitterLayoutEngine::SplitterLayoutEngine(Widget& parent) : LayoutEngine(parent) {}

SplitterLayoutEngine::~SplitterLayoutEngine() {}

void SplitterLayoutEngine::layout() {
    compute_layout();

    for (auto& item : m_items) {
        item.widget->set_positioned_rect(item.relative_rect.translated(parent().positioned_rect().top_left()));
    }
}

void SplitterLayoutEngine::do_add(Widget& widget) {
    insert_widget(widget, item_count());

    widget.on<HideEvent>(parent(), [this, &widget](auto&) {
        if (auto index = find_index_of_item(widget)) {
            remove_widget_at_index(*index);
        }
    });

    widget.on<ShowEvent>(parent(), [this, &widget](auto&) {
        if (auto index = find_index_of_item(widget)) {
            insert_widget(widget, *index);
        }
    });
}

void SplitterLayoutEngine::do_remove(Widget& widget) {
    if (widget.hidden()) {
        widget.remove_listener(parent());
        return;
    }

    auto found_index = find_index_of_item(widget);
    assert(found_index);

    remove_widget_at_index(*found_index);
}

Maybe<HoldStart> SplitterLayoutEngine::compute_hold_start(const Point& origin) const {
    for (int i = 0; i < item_count() - 1; i++) {
        auto first_item = m_items[i];
        auto second_item = m_items[i + 1];

        auto rect_between = [&]() -> Rect {
            if (direction() == Direction::Horizontal) {
                return { first_item.relative_rect.right(), 0, gutter_width(), parent().sized_rect().height() };
            }
            return { 0, first_item.relative_rect.bottom(), parent().sized_rect().width(), gutter_width() };
        }();

        if (rect_between.intersects(origin)) {
            return HoldStart { origin, i };
        }
    }
    return {};
}

void SplitterLayoutEngine::adjust_size_and_position(const HoldStart& start, const Point& drag_point) {
    Point delta = drag_point - start.origin;
    auto delta_in_layout_direction = rect_size_in_layout_direction({ 0, 0, delta.x(), delta.y() });
    if (delta_in_layout_direction == 0) {
        return;
    }

    auto min_pixel_width = [&](Widget& widget) {
        auto constraint =
            direction() == Direction::Horizontal ? widget.min_layout_constraint().width() : widget.min_layout_constraint().height();
        if (constraint == LayoutConstraint::AutoSize) {
            // FIXME: this value should depend on whether we are in libtui or libapp.
            return 8;
        }
        return constraint;
    };

    auto available_space = flexible_space();
    auto as_fraction = static_cast<double>(delta_in_layout_direction) / static_cast<double>(available_space);
    if (as_fraction < 0.0) {
        auto min_pixel_size = static_cast<double>(min_pixel_width(*m_items[start.item_index].widget));
        as_fraction = -(m_items[start.item_index].expected_fraction - max(min_pixel_size / static_cast<double>(available_space),
                                                                          m_items[start.item_index].expected_fraction + as_fraction));
    } else {
        auto min_pixel_size = static_cast<double>(min_pixel_width(*m_items[start.item_index + 1].widget));
        as_fraction = m_items[start.item_index + 1].expected_fraction - max(min_pixel_size / static_cast<double>(available_space),
                                                                            m_items[start.item_index + 1].expected_fraction - as_fraction);
    }

    m_items[start.item_index].expected_fraction += as_fraction;
    m_items[start.item_index + 1].expected_fraction -= as_fraction;

    parent().invalidate();
    schedule_layout();
}

void SplitterLayoutEngine::insert_widget(Widget& widget, int index) {
    if (widget.hidden()) {
        return;
    }

    for (auto& item : m_items) {
        item.expected_fraction *= static_cast<double>(item_count()) / static_cast<double>(item_count() + 1);
    }

    double expected_fraction = 1.0 / (item_count() + 1);
    m_items.insert({ expected_fraction, {}, widget.shared_from_this() }, index);
}

void SplitterLayoutEngine::remove_widget_at_index(int index) {
    m_items.remove(index);
    for (auto& item : m_items) {
        item.expected_fraction *= static_cast<double>(item_count() + 1) / static_cast<double>(item_count());
    }
}

Maybe<int> SplitterLayoutEngine::find_index_of_item(Widget& widget) const {
    int i = 0;
    int j = 0;
    while (i < item_count() && j < parent().children().size()) {
        if (m_items[i].widget.get() == &widget || parent().children()[j].get() == &widget) {
            return i;
        }

        if (m_items[i].widget.get() == parent().children()[j].get()) {
            i++;
            j++;
            continue;
        }

        j++;
    }

    if (i < item_count() || j < parent().children().size()) {
        return item_count();
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

int SplitterLayoutEngine::flexible_space() const {
    auto gutters = item_count() - 1;
    return max(0, available_space_in_layout_direction() - gutters * gutter_width());
}

void SplitterLayoutEngine::compute_layout() {
    if (item_count() == 0) {
        return;
    }

    auto space_available = flexible_space();

    int space_leftover = space_available;
    Point offset;
    for (auto& item : m_items) {
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
            m_hold_start->origin = mouse_position;
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
