#include <app/box_layout.h>
#include <app/widget.h>

namespace App {

BoxLayout::BoxLayout(Widget& widget) : Layout(widget) {}

BoxLayout::~BoxLayout() {}

void BoxLayout::add(SharedPtr<Widget> to_add) {
    widget().add_child(to_add);
    m_widgets.add(to_add.get());
    layout();
}

int BoxLayout::compute_available_space() const {
    return LIIM::max(0, container_rect().width() - (flexible_widget_count() - 1) * spacing() - margins().left - margins().right);
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
    int computed_width = flex_count ? available_space / flexible_widget_count() : 0;
    int computed_height = LIIM::max(0, base_rect.height() - margins().top - margins().bottom);
    int rolling_x = base_rect.x() + margins().left;
    int computed_y = base_rect.y() + margins().top;
    for (auto* w : m_widgets) {
        if (w->hidden()) {
            continue;
        }

        w->set_rect({ rolling_x, computed_y, computed_width, computed_height });
        rolling_x += computed_width + spacing();
    }
}

}
