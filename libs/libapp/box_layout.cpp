#include <app/box_layout.h>
#include <app/widget.h>

namespace App {

BoxLayout::BoxLayout(Widget& widget, Orientation orientation) : Layout(widget), m_orientation(orientation) {}

BoxLayout::~BoxLayout() {}

void BoxLayout::do_add(SharedPtr<Widget> to_add) {
    widget().add_child(to_add);
    m_widgets.add(to_add.get());
    layout();
}

int BoxLayout::compute_available_space() const {
    if (orientation() == Orientation::Horizontal) {
        return LIIM::max(0, container_rect().width() - (flexible_widget_count() - 1) * spacing() - margins().left - margins().right);
    }
    return LIIM::max(0, container_rect().height() - (flexible_widget_count() - 1) * spacing() - margins().top - margins().bottom);
}

int BoxLayout::flexible_widget_count() const {
    int count = 0;
    for (auto* w : m_widgets) {
        if (!w->hidden()) {
            count++;
        }
    }
    return count;
}

void BoxLayout::layout() {
    auto& base_rect = container_rect();

    int available_space = compute_available_space();
    int flex_count = flexible_widget_count();
    int computed_width;
    int computed_height;
    if (orientation() == Orientation::Horizontal) {
        computed_width = flex_count ? available_space / flexible_widget_count() : 0;
        computed_height = LIIM::max(0, base_rect.height() - margins().top - margins().bottom);
    } else {
        computed_width = LIIM::max(0, base_rect.width() - margins().left - margins().right);
        computed_height = flex_count ? available_space / flexible_widget_count() : 0;
    }

    int computed_x = base_rect.x() + margins().left;
    int computed_y = base_rect.y() + margins().top;
    for (auto* w : m_widgets) {
        if (w->hidden()) {
            continue;
        }

        w->set_rect({ computed_x, computed_y, computed_width, computed_height });
        if (orientation() == Orientation::Horizontal) {
            computed_x += computed_width + spacing();
        } else {
            computed_y += computed_height + spacing();
        }
    }
}

}
