#include <app/flex_layout_engine.h>
#include <gui/application.h>
#include <gui/button.h>
#include <gui/table_view.h>
#include <gui/window.h>

#include "theme_model.h"

int main() {
    auto app = GUI::Application::create();

    auto model = ThemeModel::create(nullptr);

    auto window = GUI::Window::create(nullptr, 100, 250, 400, 400, "Theme Editor");
    auto& container = window->set_main_widget<GUI::Widget>();
    auto& layout = container.set_layout_engine<App::VerticalFlexLayoutEngine>();

    auto& view = layout.add<GUI::TableView>();
    view.set_model(model);

    auto& button = layout.add<GUI::Button>("Apply Theme");
    button.set_layout_constraint({ App::LayoutConstraint::AutoSize, 24 });
    button.on<GUI::ClickEvent>({}, [&](auto&) {
        if (view.selection().empty()) {
            return;
        }

        auto& item = view.selection().first();
        app->set_global_palette(static_cast<Theme&>(item).path());
    });

    app->enter();
    return 0;
}
