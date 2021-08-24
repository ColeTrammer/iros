#include <app/layout_engine.h>
#include <graphics/rect.h>

namespace App {
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
