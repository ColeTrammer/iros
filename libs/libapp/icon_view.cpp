#include <app/icon_view.h>
#include <app/model.h>
#include <app/window.h>
#include <graphics/renderer.h>

namespace App {

void IconView::render() {
    Renderer renderer(*window()->pixels());
    for (auto& item : m_items) {
        if (auto bitmap = item.icon) {
            Rect icon_rect = { rect().x() + item.rect.x() + m_icon_padding_x, rect().y() + item.rect.y() + m_icon_padding_y, m_icon_width,
                               m_icon_height };
            renderer.draw_bitmap(*bitmap, bitmap->rect(), icon_rect);
        }
        if (!item.name.is_empty()) {
            Rect text_rect = { rect().x() + item.rect.x(), rect().y() + item.rect.y() + m_icon_height + 2 * m_icon_padding_y,
                               item.rect.width(), item.rect.height() - m_icon_height - 2 * m_icon_padding_y };
            renderer.render_text(item.name, text_rect, ColorValue::White, TextAlign::Center, font());
        }
    }
    Widget::render();
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
        if (x + w >= rect().width()) {
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
