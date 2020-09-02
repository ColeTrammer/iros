#include <app/app.h>
#include <app/box_layout.h>
#include <app/table_view.h>
#include <app/window.h>

#include "process_model.h"

int main() {
    App::App app;

    auto window = App::Window::create(nullptr, 150, 150, 400, 400, "System Monitor");
    auto model = ProcessModel::create(nullptr);
    auto& layout = window->set_layout<App::VerticalBoxLayout>();
    auto& table = layout.add<App::TableView>();
    table.set_model(model);
    app.enter();
    return 0;
}
