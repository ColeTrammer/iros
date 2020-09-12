#include <app/app.h>
#include <app/table_view.h>
#include <app/window.h>

#include "file_system_model.h"

int main() {
    App::App app;

    auto model = FileSystemModel::create(nullptr, "./");

    auto window = App::Window::create(nullptr, 350, 350, 400, 400, "File Manager");
    auto& view = window->set_main_widget<App::TableView>();
    view.set_model(model);

    app.enter();
    return 0;
}
