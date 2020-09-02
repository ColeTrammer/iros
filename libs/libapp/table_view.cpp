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
    return 2 * cell_padding() + max(width, 20);
}

void TableView::render_data(Renderer& renderer, int rx, int ry, const ModelData& data) {
    if (data.is<Monostate>()) {
        return;
    } else if (data.is<String>()) {
        auto& string = data.as<String>();
        renderer.render_text(rect().x() + rx + cell_padding(), rect().y() + ry + cell_padding(), string, ColorValue::White);
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

    int rx = 1;
    int ry = 1;
    for (auto c = 0; c < col_count; c++) {
        auto data = model()->header_data(c);
        render_data(renderer, rx, ry, data);
        rx += col_widths[c] + 1;
    }

    for (auto r = 0; r < row_count; r++) {
        rx = 1;
        ry += 21;
        for (auto c = 0; c < col_count; c++) {
            render_data(renderer, rx, ry, model()->data({ r, c }));
            rx += col_widths[c] + 1;
        }
    }

    ry = 0;
    for (int i = 0; i <= row_count; i++) {
        renderer.draw_line({ rect().x(), rect().y() + ry }, { rect().x() + rect().width(), rect().y() + ry }, ColorValue::White);
        ry += 21;
    }

    rx = 0;
    for (int i = 0; i <= col_count; i++) {
        renderer.draw_line({ rect().x() + rx, rect().y() }, { rect().x() + rx, rect().y() + rect().height() }, ColorValue::White);
        rx += col_widths.get_or(i, 0) + 1;
    }
}

}
