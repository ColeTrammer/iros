#include <app/model.h>
#include <app/table_view.h>
#include <app/window.h>
#include <graphics/renderer.h>
#include <liim/utilities.h>

namespace App {

TableView::~TableView() {}

int TableView::width_of(const ModelData& data) const {
    int width = 0;
    if (data.is<Monostate>()) {
        width = 0;
    } else if (data.is<String>()) {
        width = data.as<String>().size() * 8;
    } else if (data.is<SharedPtr<Bitmap>>()) {
        width = data.as<SharedPtr<Bitmap>>()->width();
    }
    return 2 * cell_padding() + max(width, 20);
}

void TableView::render_data(Renderer& renderer, int rx, int ry, int width, Function<ModelData(int)> getter) {
    auto cell_rect =
        Rect { rect().x() + rx + cell_padding(), rect().y() + ry + cell_padding(), width - 2 * cell_padding(), 20 - 2 * cell_padding() };
    auto data = getter(Model::Role::Display);
    if (data.is<Monostate>()) {
        return;
    } else if (data.is<String>()) {
        auto& string = data.as<String>();
        auto text_align = getter(Model::Role::TextAlignment);
        renderer.render_text(string, cell_rect, text_color(), text_align.get_or<TextAlign>(TextAlign::CenterLeft));
    } else if (data.is<SharedPtr<Bitmap>>()) {
        auto& bitmap = *data.as<SharedPtr<Bitmap>>();
        int dw = cell_rect.width() - bitmap.width();
        int dh = cell_rect.height() - bitmap.height();
        renderer.draw_bitmap(bitmap, bitmap.rect(), cell_rect.adjusted(-dw / 2, -dh / 2));
    }
}

void TableView::render() {
    if (!model()) {
        return;
    }

    Renderer renderer(*window()->pixels());
    renderer.fill_rect(rect(), background_color());
    renderer.draw_rect(rect(), outline_color());

    auto row_count = model()->row_count();
    auto col_count = model()->col_count();

    Vector<int> col_widths(col_count);
    col_widths.resize(col_count);
    for (auto c = 0; c < col_count; c++) {
        int col_width = width_of(model()->header_data(c, Model::Role::Display));
        for (auto r = 0; r < row_count; r++) {
            col_width = max(col_width, width_of(model()->data({ r, c }, Model::Role::Display)));
        }
        col_widths[c] = col_width;
    }

    int rx = 1;
    int ry = 1;
    for (auto c = 0; c < col_count; c++) {
        auto data = model()->header_data(c, Model::Role::Display);
        render_data(renderer, rx, ry, col_widths[c], [&](auto role) {
            return model()->header_data(c, role);
        });
        rx += col_widths[c] + 1;
    }

    for (auto r = 0; r < row_count; r++) {
        rx = 1;
        ry += 21;

        if (hovered_index().row() == r) {
            renderer.fill_rect({ rect().x(), rect().y() + ry, rect().width(), 21 }, ColorValue::LightGray);
        }

        for (auto c = 0; c < col_count; c++) {
            render_data(renderer, rx, ry, col_widths[c], [&](auto role) {
                return model()->data({ r, c }, role);
            });
            rx += col_widths[c] + 1;
        }
    }

    ry = 0;
    for (int i = 0; i < row_count; i++) {
        ry += 21;
        renderer.draw_line({ rect().x(), rect().y() + ry }, { rect().x() + rect().width() - 1, rect().y() + ry }, outline_color());
    }
}

ModelIndex TableView::index_at_position(int wx, int wy) {
    if (!model()) {
        return {};
    }

    auto row_count = model()->row_count();
    auto col_count = model()->col_count();

    Vector<int> col_widths(col_count);
    col_widths.resize(col_count);
    for (auto c = 0; c < col_count; c++) {
        int col_width = width_of(model()->header_data(c, Model::Role::Display));
        for (auto r = 0; r < row_count; r++) {
            col_width = max(col_width, width_of(model()->data({ r, c }, Model::Role::Display)));
        }
        col_widths[c] = col_width;
    }

    int col = -1;
    int x = 0;
    for (int i = 0; i < col_count; i++) {
        if (wx <= x) {
            col = i;
        }
        x += col_widths[i] + 1;
    }

    int row = (wy - 21) / 21;
    if (row < 0 || row >= row_count) {
        return {};
    }

    return { row, col };
}

}
