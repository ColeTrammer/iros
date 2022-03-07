#include <app/icon_view.h>
#include <app/model.h>
#include <app/window.h>
#include <eventloop/event.h>
#include <graphics/renderer.h>

namespace App {
IconView::IconView() {}

void IconView::did_attach() {
    on<MouseDownEvent>([this](const MouseDownEvent& event) {
        if (event.left_button()) {
            m_in_selection = true;
            m_selection_start = m_selection_end = { event.x(), event.y() };
            return false;
        }
        return false;
    });

    on<MouseMoveEvent>([this](const MouseMoveEvent& event) {
        if (m_in_selection) {
            selection().clear();

            m_selection_end = { event.x(), event.y() };
            Rect selection_rect = { m_selection_start, m_selection_end };
            for (auto r = 0; r < m_items.size(); r++) {
                auto& item = m_items[r];
                if (item.rect.intersects(selection_rect)) {
                    selection().add(*item.item);
                }
            }

            invalidate();
            return true;
        }
        return false;
    });

    on<MouseUpEvent>([this](const MouseUpEvent& event) {
        if (event.left_button() && m_in_selection) {
            m_in_selection = false;
            invalidate();
            return false;
        }
        return false;
    });

    on<ResizeEvent>([this](const ResizeEvent&) {
        compute_layout();
        invalidate();
    });

    on<ViewRootChanged>([this](auto&) {
        rebuild_layout();
        invalidate();
    });

    View::did_attach();
}

IconView::~IconView() {}

void IconView::render() {
    auto renderer = ScrollComponent::get_renderer();
    renderer.clear_rect(sized_rect(), background_color());

    for (int r = 0; r < m_items.size(); r++) {
        auto& item = m_items[r];
        if (auto bitmap = item.icon) {
            Rect icon_rect = { item.rect.x() + m_icon_padding_x, item.rect.y() + m_icon_padding_y, m_icon_width, m_icon_height };
            renderer.draw_bitmap(*bitmap, bitmap->rect(), icon_rect);
        }
        if (!item.name.empty()) {
            Rect text_rect = { item.rect.x(), item.rect.y() + m_icon_height + 2 * m_icon_padding_y, item.rect.width(),
                               item.rect.height() - m_icon_height - 2 * m_icon_padding_y };
            renderer.render_text(item.name, text_rect, text_color(), TextAlign::Center, *font());
        }
        if (hovered_item() == item.item) {
            renderer.draw_rect(item.rect, palette()->color(Palette::Hover));
        }
        if (selection().present(*item.item)) {
            renderer.draw_rect(item.rect, palette()->color(Palette::Selected));
        }
    }

    if (m_in_selection) {
        Rect selection_rect = { m_selection_start, m_selection_end };
        renderer.fill_rect(selection_rect, Color(106, 126, 200, 200));
        renderer.draw_rect(selection_rect, Color(76, 96, 200, 255));
    }

    Widget::render();
}

void IconView::rebuild_layout() {
    m_items.clear();

    auto* root_item = this->root_item();
    if (!root_item) {
        error_log("No root");
        return;
    }

    auto item_count = root_item->item_count();
    for (int r = 0; r < item_count; r++) {
        auto* item = root_item->model_item_at(r);
        auto info = item->info(m_name_column, ModelItemInfo::Request::Text | ModelItemInfo::Request::Bitmap);
        m_items.add({
            info.bitmap(),
            info.text().value_or(""),
            {},
            item,
        });
    }

    compute_layout();
}

void IconView::install_model_listeners(Model& model) {
    listen<ModelUpdateEvent>(model, [this, &model](auto&) {
        rebuild_layout();
    });

    View::install_model_listeners(model);

    rebuild_layout();
}

void IconView::uninstall_model_listeners(Model& model) {
    m_items.clear();
    View::uninstall_model_listeners(model);
}

void IconView::compute_layout() {
    int x = 0;
    int y = 0;
    int w = m_icon_width + 2 * m_icon_padding_x;
    int h = m_icon_height + 2 * m_icon_padding_y + 16;
    for (auto& item : m_items) {
        item.rect = { x, y, w, h };
        x += w;
        if (x + w >= positioned_rect().width()) {
            x = 0;
            y += h;
        }
    }
}

ModelItem* IconView::item_at_position(const Point& point) {
    for (auto& item : m_items) {
        if (item.rect.intersects(point)) {
            return item.item;
        }
    }
    return nullptr;
}
}
