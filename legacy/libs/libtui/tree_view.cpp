#include <app/model.h>
#include <tinput/terminal_renderer.h>
#include <tui/tree_view.h>

namespace TUI {
TreeView::TreeView() {}

void TreeView::did_attach() {
    set_padding(0);
    set_row_height(1);

    View::did_attach();
}

TreeView::~TreeView() {}

void TreeView::render() {
    auto renderer = ScrollComponent::get_renderer();
    draw_scrollbars();

    renderer.clear_rect(available_rect());

    render_items();
}

void TreeView::render_item(const App::TreeViewItem& item) {
    auto renderer = ScrollComponent::get_renderer();

    auto text_rect = item.item_rect.adjusted(-padding());
    text_rect.set_x(text_rect.x() + item.level);
    renderer.render_text(text_rect, item.name.view());
}

App::ModelItem* TreeView::item_at_position(const Point& point) {
    if (auto* item = base().internal_item_at_position(point)) {
        return item->item;
    }
    return nullptr;
}
}
