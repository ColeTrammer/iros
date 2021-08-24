#include <app/application.h>
#include <app/button.h>
#include <app/flex_layout_engine.h>
#include <app/table_view.h>
#include <app/window.h>

#include "theme_model.h"

int main() {
    auto app = App::Application::create();

    auto model = ThemeModel::create(nullptr);

    auto window = App::Window::create(nullptr, 100, 250, 400, 400, "Theme Editor");
    auto& container = window->set_main_widget<App::Widget>();
    auto& layout = container.set_layout_engine<App::VerticalFlexLayoutEngine>();

    auto& view = layout.add<App::TableView>();
    view.set_model(model);

    auto& button = layout.add<App::Button>("Apply Theme");
    button.set_layout_constraint({ App::LayoutConstraint::AutoSize, 24 });
    button.on<App::ClickEvent>({}, [&](auto&) {
        if (view.selection().empty()) {
            return;
        }

        auto index = view.selection().first();
        app->set_global_palette(model->themes()[index.row()].path);
    });

    app->enter();
    return 0;
}
