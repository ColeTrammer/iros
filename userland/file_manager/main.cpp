#include <app/app.h>
#include <app/icon_view.h>
#include <app/window.h>
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

    App::Application app;

    auto model = FileSystemModel::create(nullptr);
    model->set_base_path(starting_path);

    auto window = App::Window::create(nullptr, 350, 350, 400, 400, "File Manager");
    auto& view = window->set_main_widget<App::IconView>();
    view.set_name_column(FileSystemModel::Column::Name);
    view.set_model(model);

    view.on_item_activation = [&](const App::ModelIndex& index) {
        auto& object = model->object_from_index(index);

        fprintf(stderr, "Activated: `%s'\n", object.name.string());
        if (S_ISDIR(object.mode)) {
            model->set_base_path(model->full_path(object.name));
        }
    };

    app.enter();
    return 0;
}
