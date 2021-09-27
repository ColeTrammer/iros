#include <app/model.h>
#include <app/table_view.h>
#include <app/window.h>
#include <graphics/renderer.h>
#include <liim/utilities.h>

namespace App {
TableView::~TableView() {}

int TableView::width_of(const ModelItemInfo& info) const {
    int width = 0;
    if (info.bitmap()) {
        width = info.bitmap()->width();
    } else if (info.text()) {
        // FIXME: query the font for this information.
        width = info.text()->size() * 8;
    }
    return 2 * cell_padding() + max(width, 20);
}

void TableView::render_data(Renderer& renderer, int rx, int ry, int width, Function<ModelItemInfo()> getter) {
    auto cell_rect = Rect { rx + cell_padding(), ry + cell_padding(), width - 2 * cell_padding(), 20 - 2 * cell_padding() };
    auto info = getter();

    if (info.bitmap()) {
        auto& bitmap = *info.bitmap();
        int dw = cell_rect.width() - bitmap.width();
        int dh = cell_rect.height() - bitmap.height();
        renderer.draw_bitmap(bitmap, bitmap.rect(), cell_rect.adjusted(-dw / 2, -dh / 2));
    } else if (info.text()) {
        auto& string = *info.text();
        auto text_align = info.text_align().value_or(TextAlign::CenterLeft);
        renderer.render_text(string, cell_rect, text_color(), text_align);
    }
}

void TableView::render() {
    if (!model()) {
        return;
    }

    auto renderer = get_renderer();
    renderer.fill_rect(sized_rect(), background_color());

    auto dimensions = model()->dimensions();

    auto col_widths = Vector<int> { dimensions.field_count };
    col_widths.resize(dimensions.field_count);
    for (auto c = 0; c < dimensions.field_count; c++) {
        int col_width = width_of(model()->header_info(c, ModelItemInfo::Request::Text));
        for (auto r = 0; r < dimensions.item_count; r++) {
            col_width =
                max(col_width, width_of(model()->item_info({ r, c }, ModelItemInfo::Request::Text | ModelItemInfo::Request::Bitmap)));
        }
        col_widths[c] = col_width;
    }

    int rx = 1;
    int ry = 1;
    for (auto c = 0; c < dimensions.field_count; c++) {
        auto data = model()->header_info(c, ModelItemInfo::Request::Text);
        render_data(renderer, rx, ry, col_widths[c], [&]() {
            return model()->header_info(c, ModelItemInfo::Request::Text | ModelItemInfo::Request::TextAlign);
        });
        rx += col_widths[c] + 1;
    }

    for (auto r = 0; r < dimensions.item_count; r++) {
        rx = 1;
        ry += 21;

        if (hovered_index().item() == r) {
            renderer.fill_rect({ 0, ry, sized_rect().width(), 21 }, palette()->color(Palette::Hover));
        }

        if (selection().present({ r, 0 })) {
            renderer.fill_rect({ 0, ry, sized_rect().width(), 21 }, palette()->color(Palette::Selected));
        }

        for (auto c = 0; c < dimensions.field_count; c++) {
            render_data(renderer, rx, ry, col_widths[c], [&]() {
                return model()->item_info({ r, c }, ModelItemInfo::Request::Text | ModelItemInfo::Request::Bitmap |
                                                        ModelItemInfo::Request::TextAlign);
            });
            rx += col_widths[c] + 1;
        }
    }

    ry = 0;
    for (int i = 0; i <= dimensions.item_count; i++) {
        ry += 21;
        renderer.draw_line({ 0, ry }, { sized_rect().right() - 1, ry }, outline_color());
    }
    renderer.draw_rect(sized_rect(), outline_color());
}

ModelIndex TableView::index_at_position(int wx, int wy) {
    if (!model()) {
        return {};
    }

    auto dimensions = model()->dimensions();

    auto col_widths = Vector<int> { dimensions.field_count };
    col_widths.resize(dimensions.field_count);
    for (auto c = 0; c < dimensions.field_count; c++) {
        int col_width = width_of(model()->header_info(c, ModelItemInfo::Request::Text));
        for (auto r = 0; r < dimensions.item_count; r++) {
            col_width =
                max(col_width, width_of(model()->item_info({ r, c }, ModelItemInfo::Request::Text | ModelItemInfo::Request::Bitmap)));
        }
        col_widths[c] = col_width;
    }

    int field = -1;
    int x = 0;
    for (int i = 0; i < dimensions.field_count; i++) {
        x += col_widths[i] + 1;
        if (wx <= x) {
            field = i;
        }
    }

    int item = (wy - 21) / 21;
    if (item < 0 || item >= dimensions.item_count || field == -1) {
        return {};
    }

    return { item, field };
}
}
