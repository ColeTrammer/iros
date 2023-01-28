#include <app/model.h>
#include <app/model_item_info.h>
#include <tinput/terminal_renderer.h>
#include <tui/table_view.h>

namespace TUI {
void TableView::render() {
    auto renderer = ScrollComponent::get_renderer();
    if (!model()) {
        renderer.clear_rect(sized_rect());
        return;
    }

    auto* root_item = this->root_item();
    auto item_count = root_item->item_count();
    for (int i = 0; i < item_count; i++) {
        auto info = root_item->model_item_at(i)->info(0, App::ModelItemInfo::Request::Text);
        renderer.render_text(sized_rect().with_y(i).with_height(1), info.text().value_or("").view());
    }
}
}
