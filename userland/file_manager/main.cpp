#include <app/application.h>
#include <app/button.h>
#include <app/flex_layout_engine.h>
#include <app/icon_view.h>
#include <app/window.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>

#include "file_system_model.h"

static void print_usage_and_exit(const char* s) {
    fprintf(stderr, "Usage: %s [path]\n", s);
    exit(2);
}

int main(int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, ":")) != -1) {
        switch (opt) {
            case ':':
            case '?':
                print_usage_and_exit(*argv);
        }
    }

    const char* starting_path = "./";
    if (argc - optind >= 2) {
        print_usage_and_exit(*argv);
    } else if (argc - optind == 1) {
        starting_path = argv[optind];
    }

    auto app = App::Application::create();

    auto model = FileSystemModel::create(nullptr);
    model->set_base_path(starting_path);

    auto window = App::Window::create(nullptr, 350, 350, 400, 400, "File Manager");
    auto& main_widget = window->set_main_widget<App::Widget>();
    auto& layout = main_widget.set_layout_engine<App::VerticalFlexLayoutEngine>();

    auto& parent_button = layout.add<App::Button>("Go to parent");
    parent_button.set_layout_constraint({ 100, 24 });
    parent_button.on<App::ClickEvent>({}, [&](auto&) {
        model->go_to_parent();
    });

    auto& view = layout.add<App::IconView>();
    view.set_name_column(FileSystemModel::Column::Name);
    view.set_model(model);

    view.on_item_activation = [&](const App::ModelIndex& index) {
        auto& object = model->model_item_root()->typed_item<FileSystemObject>(index.item());

        fprintf(stderr, "Activated: `%s'\n", object.name().string());
        if (object.mode() & S_IFDIR) {
            model->set_base_path(model->full_path(object).to_string());
        }
    };

    app->enter();
    return 0;
}
