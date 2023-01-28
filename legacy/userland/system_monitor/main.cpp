#include <gui/application.h>
#include <gui/tab_widget.h>
#include <gui/window.h>

#include "process_model.h"
#include "process_tab.h"
#include "resource_usage_tab.h"

int main() {
    auto app = GUI::Application::create();

    auto window = GUI::Window::create(nullptr, 150, 150, 600, 400, "System Monitor");
    auto& tabs = window->set_main_widget<GUI::TabWidget>();

    auto model = ProcessModel::create(nullptr);

    tabs.add_tab<ProcessTab>("Processes", model);
    tabs.add_tab<ResourceUsageTab>("Resources", model);

    app->enter();
    return 0;
}
