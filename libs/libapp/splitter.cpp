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

void SplitterLayoutEngine::add_impl(Maybe<int> fixed_width, Widget& widget) {
    if (fixed_width) {
        if (direction() == Direction::Horizontal) {
            widget.set_layout_constraint({ *fixed_width, LayoutConstraint::AutoSize });
        } else {
            widget.set_layout_constraint({ LayoutConstraint::AutoSize, *fixed_width });
        }
    }

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
    compute_layout();

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

    auto& first_item = m_items[start.item_index];
    auto& latter_item = m_items[start.item_index + 1];

    auto shrink_item = [&](Item& item, int& pixels) {
        auto min_pixel_size = min_pixel_width(*item.widget);
        pixels = min(pixels, rect_size_in_layout_direction(item.relative_rect) - min_pixel_size);

        if (is_flexible_item(item)) {
            auto as_fraction = static_cast<double>(pixels) / available_space;
            item.expected_fraction -= as_fraction;
        } else {
            if (direction() == Direction::Horizontal) {
                item.widget->set_layout_constraint(
                    { item.widget->layout_constraint().width() - pixels, item.widget->layout_constraint().height() });
            } else {
                item.widget->set_layout_constraint(
                    { item.widget->layout_constraint().width(), item.widget->layout_constraint().height() - pixels });
            }
        }
    };

    auto expand_item = [&](Item& item, int pixels) {
        if (is_flexible_item(item)) {
            auto as_fraction = static_cast<double>(pixels) / available_space;
            item.expected_fraction += as_fraction;
        } else {
            if (direction() == Direction::Horizontal) {
                item.widget->set_layout_constraint(
                    { item.widget->layout_constraint().width() + pixels, item.widget->layout_constraint().height() });
            } else {
                item.widget->set_layout_constraint(
                    { item.widget->layout_constraint().width(), item.widget->layout_constraint().height() + pixels });
            }
        }
    };

    if (delta_in_layout_direction < 0) {
        auto to_shrink = -delta_in_layout_direction;
        shrink_item(first_item, to_shrink);
        expand_item(latter_item, to_shrink);
    } else {
        shrink_item(latter_item, delta_in_layout_direction);
        expand_item(first_item, delta_in_layout_direction);
    }

    parent().invalidate();
    schedule_layout();
}

void SplitterLayoutEngine::insert_widget(Widget& widget, int index) {
    if (widget.hidden()) {
        return;
    }

    auto flex_item_count = flexible_item_count();

    double expected_fraction = 1.0 / (flex_item_count + 1);
    auto new_item = Item { expected_fraction, {}, widget.shared_from_this() };

    if (is_flexible_item(new_item)) {
        for (auto& item : m_items) {
            if (is_flexible_item(item)) {
                item.expected_fraction -= expected_fraction * item.expected_fraction;
            }
        }
    }

    m_items.insert(move(new_item), index);
}

void SplitterLayoutEngine::remove_widget_at_index(int index) {
    bool was_flexible_item = is_flexible_item(m_items[index]);
    double old_expected_fraction = m_items[index].expected_fraction;
    m_items.remove(index);

    if (was_flexible_item) {
        for (auto& item : m_items) {
            if (is_flexible_item(item)) {
                item.expected_fraction += old_expected_fraction / (1.0 - old_expected_fraction) * item.expected_fraction;
            }
        }
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

int SplitterLayoutEngine::flexible_item_count() const {
    int count = 0;
    for (auto& item : m_items) {
        if (is_flexible_item(item)) {
            count++;
        }
    }
    return count;
}

bool SplitterLayoutEngine::is_flexible_item(const Item& item) const {
    return rect_size_in_layout_direction({ 0, 0, item.widget->layout_constraint().width(), item.widget->layout_constraint().height() }) ==
           LayoutConstraint::AutoSize;
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
    auto base_flexible_space = available_space_in_layout_direction() - gutters * gutter_width();
    for (auto& item : m_items) {
        if (!is_flexible_item(item)) {
            base_flexible_space -= rect_size_in_layout_direction(
                { 0, 0, item.widget->layout_constraint().width(), item.widget->layout_constraint().height() });
        }
    }
    return max(0, base_flexible_space);
}

void SplitterLayoutEngine::compute_layout() {
    if (item_count() == 0) {
        return;
    }

    auto space_available = flexible_space();
    auto flex_item_count = flexible_item_count();

    // FIXME: potenially consider this layout's margins.
    Point offset;

    int space_leftover = space_available;
    for (auto& item : m_items) {
        if (is_flexible_item(item)) {
            auto length = flex_item_count > 1 ? static_cast<int>(space_available * item.expected_fraction) : space_leftover;
            auto width = direction() == Direction::Horizontal ? length : available_space_against_layout_direction();
            auto height = direction() == Direction::Vertical ? length : available_space_against_layout_direction();

            item.relative_rect = { offset.x(), offset.y(), width, height };
            space_leftover -= length;
            flex_item_count--;
        } else {
            auto constraint = item.widget->layout_constraint();
            auto width = direction() == Direction::Horizontal ? constraint.width() : parent().sized_rect().width();
            auto height = direction() == Direction::Vertical ? constraint.height() : parent().sized_rect().height();
            item.relative_rect = { offset.x(), offset.y(), width, height };
        }

        if (direction() == Direction::Horizontal) {
            offset.set_x(offset.x() + item.relative_rect.width() + gutter_width());
        } else {
            offset.set_y(offset.y() + item.relative_rect.height() + gutter_width());
        }
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
