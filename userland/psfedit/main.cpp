#include <app/app.h>
#include <app/box_layout.h>
#include <app/button.h>
#include <app/text_label.h>
#include <app/widget.h>
#include <app/window.h>
#include <graphics/font.h>

int main() {
    App::App app;

    auto window = App::Window::create(nullptr, 300, 300, 400, 400, "PSF Edit");

    auto& layout = window->set_layout<App::BoxLayout>(App::BoxLayout::Orientation::Vertical);
    layout.set_spacing(0);

    for (int i = 0; i < 16; i++) {
        auto& row_widget = layout.add<App::Widget>();
        auto& row_layout = row_widget.set_layout<App::BoxLayout>(App::BoxLayout::Orientation::Horizontal);
        row_layout.set_margins({ 0, 0, 0, 0 });
        row_layout.set_spacing(0);
        for (int j = 0; j < 16; j++) {
            auto& button = row_layout.add<App::Button>(String(static_cast<char>(i * 16 + j)));
            button.on_click = [i, j]() {
                fprintf(stderr, "%d\n", i * 16 + j);
            };
        }
    }

    window->draw();
    app.enter();
    return 0;
}
