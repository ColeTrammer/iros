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
    }
    return 2 + 2 + max(width, 20);
}

void TableView::render_data(Renderer& renderer, int rx, int ry, const ModelData& data) {
    if (data.is<Monostate>()) {
        return;
    } else if (data.is<String>()) {
        auto& string = data.as<String>();
        renderer.render_text(rect().x() + rx + 2, rect().y() + ry + 2, string, ColorValue::White);
    }
}

void TableView::render() {
    if (!model()) {
        return;
    }

    Renderer renderer(*window()->pixels());
    renderer.fill_rect(rect(), ColorValue::Black);

    auto row_count = model()->row_count();
    auto col_count = model()->col_count();

    Vector<int> col_widths(col_count);
    col_widths.resize(col_count);
    for (auto c = 0; c < col_count; c++) {
        int col_width = width_of(model()->header_data(c));
        for (auto r = 0; r < row_count; r++) {
            col_width = max(col_width, width_of(model()->data({ r, c })));
        }
        col_widths[c] = col_width;
    }

    int rx = 0;
    int ry = 0;
    for (auto c = 0; c < col_count; c++) {
        auto data = model()->header_data(c);
        render_data(renderer, rx, ry, data);
        rx += col_widths[c];
    }

    for (auto r = 0; r < row_count; r++) {
        rx = 0;
        ry += 20;
        for (auto c = 0; c < col_count; c++) {
            render_data(renderer, rx, ry, model()->data({ r, c }));
            rx += col_widths[c];
        }
    }
}

}
