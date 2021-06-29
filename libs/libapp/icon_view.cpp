#include <app/icon_view.h>
#include <app/model.h>
#include <app/window.h>
#include <eventloop/event.h>
#include <graphics/renderer.h>

namespace App {
void IconView::render() {
    auto renderer = get_renderer();
    renderer.clear_rect(sized_rect(), background_color());

    for (int r = 0; r < m_items.size(); r++) {
        auto& item = m_items[r];
        if (auto bitmap = item.icon) {
            Rect icon_rect = { item.rect.x() + m_icon_padding_x, item.rect.y() + m_icon_padding_y, m_icon_width, m_icon_height };
            renderer.draw_bitmap(*bitmap, bitmap->rect(), icon_rect);
        }
        if (!item.name.is_empty()) {
            Rect text_rect = { item.rect.x(), item.rect.y() + m_icon_height + 2 * m_icon_padding_y, item.rect.width(),
                               item.rect.height() - m_icon_height - 2 * m_icon_padding_y };
            renderer.render_text(item.name, text_rect, text_color(), TextAlign::Center, font());
        }
        if (hovered_index() == ModelIndex { r, m_name_column }) {
            renderer.draw_rect(item.rect, palette()->color(Palette::Hover));
        }
        if (is_selected(ModelIndex { r, m_name_column })) {
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

void IconView::on_mouse_event(MouseEvent& event) {
    if (event.mouse_event_type() == MouseEventType::Down) {
        if (event.button() == MouseButton::Left) {
            m_in_selection = true;
            m_selection_start = m_selection_end = { event.x(), event.y() };
        }
    } else if (event.mouse_event_type() == MouseEventType::Move) {
        if (m_in_selection) {
            clear_selection();

            m_selection_end = { event.x(), event.y() };
            Rect selection_rect = { m_selection_start, m_selection_end };
            for (auto r = 0; r < m_items.size(); r++) {
                auto& item = m_items[r];
                if (item.rect.intersects(selection_rect)) {
                    add_to_selection({ r, m_name_column });
                }
            }

            invalidate();
        }
    } else if (event.mouse_event_type() == MouseEventType::Up) {
        if (m_in_selection) {
            m_in_selection = false;
            invalidate();
        }
    }

    return View::on_mouse_event(event);
}

void IconView::on_resize() {
    compute_layout();
    invalidate();
}

void IconView::model_did_update() {
    m_items.clear();

    for (int r = 0; r < model()->row_count(); r++) {
        m_items.add({
            model()->data({ r, m_name_column }, Model::Role::Icon).get_or<SharedPtr<Bitmap>>(nullptr),
            model()->data({ r, m_name_column }, Model::Role::Display).get_or<String>(""),
            {},
            { r, m_name_column },
        });
    }

    compute_layout();
    View::model_did_update();
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

ModelIndex IconView::index_at_position(int x, int y) {
    for (auto& item : m_items) {
        if (item.rect.intersects({ x, y })) {
            return item.index;
        }
    }
    return {};
}
}
