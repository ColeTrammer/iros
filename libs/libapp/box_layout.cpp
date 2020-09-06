#include <app/box_layout.h>
#include <app/widget.h>

namespace App {

BoxLayout::BoxLayout(Widget& widget, Orientation orientation) : Layout(widget), m_orientation(orientation) {}

BoxLayout::~BoxLayout() {}

void BoxLayout::do_add(SharedPtr<Widget> to_add) {
    m_widgets.add(to_add.get());
    layout();
}

int BoxLayout::compute_available_space() const {
    int total_preferred_size = 0;
    for (auto* w : m_widgets) {
        if (!w->hidden()) {
            auto relevant_size = orientation() == Orientation::Horizontal ? w->preferred_size().width : w->preferred_size().height;
            if (relevant_size != Size::Auto) {
                total_preferred_size += relevant_size;
            }
        }
    }

    auto visible_count = visible_widget_count();
    if (orientation() == Orientation::Horizontal) {
        return LIIM::max(0, container_rect().width() - (visible_count - 1) * spacing() - margins().left - margins().right -
                                total_preferred_size);
    }
    return LIIM::max(0,
                     container_rect().height() - (visible_count - 1) * spacing() - margins().top - margins().bottom - total_preferred_size);
}

int BoxLayout::visible_widget_count() const {
    int count = 0;
    for (auto* w : m_widgets) {
        if (!w->hidden()) {
            count++;
        }
    }
    return count;
}

int BoxLayout::flexible_widget_count() const {
    int count = 0;
    for (auto* w : m_widgets) {
        if (!w->hidden()) {
            auto relevant_size = orientation() == Orientation::Horizontal ? w->preferred_size().width : w->preferred_size().height;
            if (relevant_size == Size::Auto) {
                count++;
            }
        }
    }
    return count;
}

void BoxLayout::layout() {
    auto& base_rect = container_rect();

    bool respect_preffered_sizes = true;
    int available_space = compute_available_space();
    int flex_count;
    if (available_space > 0) {
        flex_count = flexible_widget_count();
    } else {
        flex_count = visible_widget_count();
        if (orientation() == Orientation::Horizontal) {
            available_space = LIIM::max(0, container_rect().width() - (flex_count - 1) * spacing() - margins().left - margins().right);
        } else {
            available_space = LIIM::max(0, container_rect().height() - (flex_count - 1) * spacing() - margins().top - margins().bottom);
        }
        respect_preffered_sizes = false;
    }

    int computed_width;
    int computed_height;
    if (orientation() == Orientation::Horizontal) {
        computed_width = flex_count ? available_space / flex_count : 0;
        computed_height = LIIM::max(0, base_rect.height() - margins().top - margins().bottom);
    } else {
        computed_width = LIIM::max(0, base_rect.width() - margins().left - margins().right);
        computed_height = flex_count ? available_space / flex_count : 0;
    }

    int computed_x = base_rect.x() + margins().left;
    int computed_y = base_rect.y() + margins().top;
    for (auto* w : m_widgets) {
        if (w->hidden()) {
            continue;
        }

        auto size_in_layout_orientation = orientation() == Orientation::Horizontal ? w->preferred_size().width : w->preferred_size().height;
        if (orientation() == Orientation::Horizontal) {
            auto used_width =
                (size_in_layout_orientation == Size::Auto || !respect_preffered_sizes) ? computed_width : size_in_layout_orientation;
            w->set_rect({ computed_x, computed_y, used_width, computed_height });
            computed_x += used_width + spacing();
        } else {
            auto used_height =
                (size_in_layout_orientation == Size::Auto || !respect_preffered_sizes) ? computed_height : size_in_layout_orientation;
            w->set_rect({ computed_x, computed_y, computed_width, used_height });
            computed_y += used_height + spacing();
        }
    }
}

}
