#include <app/base/application.h>
#include <app/layout_engine.h>
#include <graphics/rect.h>

namespace App {
LayoutEngine::LayoutEngine(Base::Widget& parent) : m_parent(parent) {
    set_margins(App::Base::Application::the().default_margins());
}

void LayoutEngine::schedule_layout() {
    parent().deferred_invoke_batched(m_layout_scheduled, [this] {
        // NOTE: instead of this check, it would perhaps be better for
        //       LayoutEngine to be an App::Object.
        if (parent().layout_engine() == this) {
            layout();
        }
    });
}

Rect LayoutEngine::parent_rect() const {
    auto rect = parent().positioned_rect();
    if (margins().top + margins().bottom >= rect.height()) {
        return {};
    }
    if (margins().left + margins().right >= rect.width()) {
        return {};
    }
    return rect.with_x(rect.x() + margins().left)
        .with_width(rect.width() - margins().right)
        .with_y(rect.y() + margins().top)
        .with_height(rect.height() - margins().bottom);
}
}
