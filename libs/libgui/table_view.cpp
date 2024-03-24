#include <app/model.h>
#include <graphics/renderer.h>
#include <gui/table_view.h>
#include <gui/window.h>
#include <liim/utilities.h>
#include <sys/wait.h>

namespace GUI {
TableView::~TableView() {}

int TableView::width_of(const App::ModelItemInfo& info) const {
    int width = 0;
    if (info.bitmap()) {
        width = info.bitmap()->width();
    } else if (info.text()) {
        // FIXME: query the font for this information.
        width = info.text()->size() * 8;
    }
    return 2 * cell_padding() + max(width, 20);
}

void TableView::render_data(Renderer& renderer, int rx, int ry, int width, Function<App::ModelItemInfo()> getter) {
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

    auto root_item = this->root_item();
    if (!root_item) {
        return;
    }

    auto renderer = ScrollComponent::get_renderer();
    renderer.fill_rect(sized_rect(), background_color());

    auto field_count = model()->field_count();
    auto item_count = root_item->item_count();

    auto col_widths = Vector<int> { field_count };
    col_widths.resize(field_count);
    for (auto c = 0; c < field_count; c++) {
        int col_width = width_of(model()->header_info(c, App::ModelItemInfo::Request::Text));
        for (auto r = 0; r < item_count; r++) {
            col_width = max(col_width, width_of(root_item->model_item_at(r)->info(c, App::ModelItemInfo::Request::Text |
                                                                                         App::ModelItemInfo::Request::Bitmap)));
        }
        col_widths[c] = col_width;
    }

    int rx = 1;
    int ry = 1;
    for (auto c = 0; c < field_count; c++) {
        auto data = model()->header_info(c, App::ModelItemInfo::Request::Text);
        render_data(renderer, rx, ry, col_widths[c], [&]() {
            return model()->header_info(c, App::ModelItemInfo::Request::Text | App::ModelItemInfo::Request::TextAlign);
        });
        rx += col_widths[c] + 1;
    }

    for (auto r = 0; r < item_count; r++) {
        rx = 1;
        ry += 21;

        auto* item = root_item->model_item_at(r);
        if (hovered_item() == item) {
            renderer.fill_rect({ 0, ry, sized_rect().width(), 21 }, palette()->color(Palette::Hover));
        }

        if (selection().present(*item)) {
            renderer.fill_rect({ 0, ry, sized_rect().width(), 21 }, palette()->color(Palette::Selected));
        }

        for (auto c = 0; c < field_count; c++) {
            render_data(renderer, rx, ry, col_widths[c], [&]() {
                return item->info(c, App::ModelItemInfo::Request::Text | App::ModelItemInfo::Request::Bitmap |
                                         App::ModelItemInfo::Request::TextAlign);
            });
            rx += col_widths[c] + 1;
        }
    }

    ry = 0;
    for (int i = 0; i <= item_count; i++) {
        ry += 21;
        renderer.draw_line({ 0, ry }, { sized_rect().right() - 1, ry }, outline_color());
    }
    renderer.draw_rect(sized_rect(), outline_color());
}

App::ModelItem* TableView::item_at_position(const Point& point) {
    if (!model()) {
        return nullptr;
    }

    auto root_item = this->root_item();
    if (!root_item) {
        return nullptr;
    }

    auto item_count = root_item->item_count();
    auto ry = 1;
    for (auto r = 0; r < item_count; r++) {
        ry += 21;

        if (point.y() < ry + 21 && point.y() >= ry) {
            return root_item->model_item_at(r);
        }
    }

    return nullptr;
}
}
