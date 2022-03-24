#include <app/model.h>
#include <app/tree_view.h>
#include <graphics/renderer.h>

namespace App {
TreeView::TreeView() {}

void TreeView::did_attach() {
    set_padding(2);
    set_row_height(32);

    View::did_attach();
}

TreeView::~TreeView() {}

void TreeView::render_item(const Base::TreeViewItem& item) {
    auto renderer = ScrollComponent::get_renderer();
    renderer.draw_rect(item.item_rect, outline_color());

    auto text_rect = item.item_rect.adjusted(-padding());
    text_rect.set_x(text_rect.x() + 16 * item.level);
    renderer.render_text(item.name, text_rect, text_color());
}

void TreeView::render() {
    auto renderer = ScrollComponent::get_renderer();
    draw_scrollbars();

    renderer.clear_rect(available_rect(), background_color());

    render_items();
}

ModelItem* TreeView::item_at_position(const Point& point) {
    if (auto* item = base().internal_item_at_position(point)) {
        return item->item;
    }
    return nullptr;
}
}
