#include <app/app.h>
#include <app/box_layout.h>
#include <app/button.h>
#include <app/table_view.h>
#include <app/window.h>

#include "theme_model.h"

int main() {
    App::App app;

    auto model = ThemeModel::create(nullptr);

    auto window = App::Window::create(nullptr, 100, 250, 400, 400, "Theme Editor");
    auto& container = window->set_main_widget<App::Widget>();
    auto& layout = container.set_layout<App::VerticalBoxLayout>();

    auto& view = layout.add<App::TableView>();
    view.set_model(model);

    auto& button = layout.add<App::Button>("Apply Theme");
    button.set_preferred_size({ App::Size::Auto, 24 });
    button.on_click = [&] {
        if (view.selection().empty()) {
            return;
        }

        auto index = view.selection().first();
        app.ws().server().send_then_wait<WindowServer::Client::ChangeThemeRequest, WindowServer::Server::ChangeThemeResponse>(
            { .path = model->themes()[index.row()].path });
    };

    app.enter();
    return 0;
}
