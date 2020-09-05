#include <app/app.h>
#include <app/box_layout.h>
#include <app/tab_widget.h>
#include <app/table_view.h>
#include <app/text_label.h>
#include <app/window.h>

#include "process_model.h"

int main() {
    App::App app;

    auto window = App::Window::create(nullptr, 150, 150, 600, 400, "System Monitor");
    auto& tabs = window->set_main_widget<App::TabWidget>();

    auto model = ProcessModel::create(nullptr);
    auto& table = tabs.add_tab<App::TableView>("Processes");
    table.set_model(model);

    tabs.add_tab<App::TextLabel>("Resources", "CPU");

    app.enter();
    return 0;
}
