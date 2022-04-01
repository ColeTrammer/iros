#include <app/base/tree_view.h>
#include <app/model.h>

namespace App::Base {
TreeView::TreeView(SharedPtr<WidgetBridge> widget_bridge, SharedPtr<ViewBridge> view_bridge,
                   SharedPtr<ScrollComponentBridge> scroll_component_bridge, SharedPtr<TreeViewBridge> tree_view_bridge)
    : View(move(widget_bridge), move(view_bridge), move(scroll_component_bridge)), m_bridge(move(tree_view_bridge)) {}

void TreeView::initialize() {
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

    View::initialize();
}

TreeView::~TreeView() {}

void TreeView::render_items() {
    Function<void(const Vector<TreeViewItem>&)> render_level = [&](auto& items) {
        for (auto& item : items) {
            render_item(item);
            if (item.open) {
                render_level(item.children);
            }
        }
    };

    render_level(m_items);
}

TreeViewItem* TreeView::internal_item_at_position(const Point& point) {
    Function<TreeViewItem*(Vector<TreeViewItem>&)> find = [&](Vector<TreeViewItem>& items) -> TreeViewItem* {
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

    Function<TreeViewItem(ModelItem*, int)> process_item = [&](ModelItem* item, int level) {
        auto info = item->info(name_column(), ModelItemInfo::Request::Text);

        Vector<TreeViewItem> children;
        auto item_count = item->item_count();
        for (int r = 0; r < item_count; r++) {
            children.add(process_item(item->model_item_at(r), level + 1));
        }

        return TreeViewItem { info.text().value_or(""), {}, {}, item, level, false, move(children) };
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

    Function<void(TreeViewItem&)> process_item = [&](TreeViewItem& item) {
        auto initial_y = y;
        y += row_height();

        if (item.open) {
            for (auto& child : item.children) {
                process_item(child);
            }
        }

        item.item_rect = { 0, initial_y, available_rect().width(), row_height() };
        item.container_rect = { 0, initial_y, available_rect().width(), y - initial_y };
    };

    for (auto& item : m_items) {
        process_item(item);
    }

    set_layout_constraint({ layout_constraint().width(), y });
    invalidate();
}
}
