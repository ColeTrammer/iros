#include <app/model.h>
#include <app/tree_view.h>
#include <graphics/renderer.h>

namespace App {
void TreeView::initialize() {
    on<ResizeEvent>([this](auto&) {
        rebuild_layout();
        invalidate();
    });

    on<ViewRootChanged>([this](auto&) {
        rebuild_layout();
        invalidate();
    });

    View::initialize();
}

void TreeView::render() {
    auto renderer = ScrollComponent::get_renderer();
    draw_scrollbars();

    renderer.clear_rect(available_rect(), background_color());

    auto render_item = [&](auto& item) {
        renderer.draw_rect(item.item_rect, outline_color());

        auto text_rect = item.item_rect.adjusted(-m_padding);
        text_rect.set_x(text_rect.x() + 16 * item.level);
        renderer.render_text(item.name, text_rect, text_color());
    };

    Function<void(const Vector<Item>&)> render_level = [&](auto& items) {
        for (auto& item : items) {
            render_item(item);
            render_level(item.children);
        }
    };

    render_level(m_items);
}

ModelItem* TreeView::item_at_position(const Point&) {
    return nullptr;
}

void TreeView::install_model_listeners(Model& model) {
    model.on<ModelUpdateEvent>(*this, [this, &model](auto&) {
        rebuild_layout();
    });

    View::install_model_listeners(model);

    rebuild_layout();
}

void TreeView::uninstall_model_listeners(Model& model) {
    m_items.clear();
    View::uninstall_model_listeners(model);
}

void TreeView::rebuild_layout() {
    m_items.clear();

    int y = 0;

    Function<Item(ModelItem*, int)> process_item = [&](ModelItem* item, int level) {
        auto info = item->info(m_name_column, ModelItemInfo::Request::Text);

        auto initial_y = y;
        y += m_row_height;

        Vector<Item> children;
        auto item_count = item->item_count();
        for (int r = 0; r < item_count; r++) {
            m_items.add(process_item(item->model_item_at(r), level + 1));
        }

        return Item { info.text().value_or(""),
                      Rect { 0, initial_y, available_rect().width(), m_row_height },
                      Rect { 0, initial_y, available_rect().width(), y - initial_y },
                      item,
                      level,
                      true,
                      move(children) };
    };

    auto* root_item = this->root_item();
    if (!root_item) {
        return;
    }

    auto item_count = root_item->item_count();
    for (int r = 0; r < item_count; r++) {
        auto* item = root_item->model_item_at(r);
        m_items.add(process_item(item, 0));
    }

    set_layout_constraint({ LayoutConstraint::AutoSize, m_items.size() * m_row_height });
}
}
