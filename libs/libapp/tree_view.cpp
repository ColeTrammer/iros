#include <app/model.h>
#include <app/tree_view.h>
#include <graphics/renderer.h>

namespace App {
void TreeView::did_attach() {
    on<ResizeEvent>([this](auto&) {
        rebuild_layout();
    });

    on<ViewRootChanged>([this](auto&) {
        rebuild_items();
    });

    on<MouseDownEvent>([this](const MouseDownEvent& event) {
        auto* item = internal_item_at_position({ event.x(), event.y() });
        if (!item) {
            return false;
        }

        if (event.left_button()) {
            item->open = !item->open;
            rebuild_layout();
            return true;
        }
        return false;
    });

    View::did_attach();
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
            if (item.open) {
                render_level(item.children);
            }
        }
    };

    render_level(m_items);
}

ModelItem* TreeView::item_at_position(const Point& point) {
    if (auto* item = internal_item_at_position(point)) {
        return item->item;
    }
    return nullptr;
}

TreeView::Item* TreeView::internal_item_at_position(const Point& point) {
    Function<Item*(Vector<Item>&)> find = [&](Vector<Item>& items) -> Item* {
        for (auto& item : items) {
            if (item.item_rect.intersects(point)) {
                return &item;
            } else if (item.container_rect.intersects(point)) {
                return find(item.children);
            }
        }
        return nullptr;
    };

    return find(m_items);
}

void TreeView::install_model_listeners(Model& model) {
    listen<ModelUpdateEvent>(model, [this, &model](auto&) {
        rebuild_items();
    });

    View::install_model_listeners(model);

    rebuild_items();
}

void TreeView::uninstall_model_listeners(Model& model) {
    m_items.clear();
    View::uninstall_model_listeners(model);
}

void TreeView::rebuild_items() {
    m_items.clear();

    Function<Item(ModelItem*, int)> process_item = [&](ModelItem* item, int level) {
        auto info = item->info(m_name_column, ModelItemInfo::Request::Text);

        Vector<Item> children;
        auto item_count = item->item_count();
        for (int r = 0; r < item_count; r++) {
            children.add(process_item(item->model_item_at(r), level + 1));
        }

        return Item { info.text().value_or(""), {}, {}, item, level, false, move(children) };
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

    rebuild_layout();
}

void TreeView::rebuild_layout() {
    int y = 0;

    Function<void(Item&)> process_item = [&](Item& item) {
        auto initial_y = y;
        y += m_row_height;

        if (item.open) {
            for (auto& child : item.children) {
                process_item(child);
            }
        }

        item.item_rect = { 0, initial_y, available_rect().width(), m_row_height };
        item.container_rect = { 0, initial_y, available_rect().width(), y - initial_y };
    };

    for (auto& item : m_items) {
        process_item(item);
    }

    set_layout_constraint({ LayoutConstraint::AutoSize, y });
    invalidate();
}
}
