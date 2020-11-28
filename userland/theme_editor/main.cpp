#include <app/app.h>
#include <app/table_view.h>
#include <app/window.h>

#include "theme_model.h"

int main() {
    App::App app;

    auto model = ThemeModel::create(nullptr);

    auto window = App::Window::create(nullptr, 100, 250, 400, 400, "Theme Editor");
    auto& view = window->set_main_widget<App::TableView>();
    view.set_model(model);

    app.enter();
    return 0;
}
