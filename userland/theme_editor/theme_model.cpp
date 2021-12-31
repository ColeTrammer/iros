#include <dirent.h>
#include <sys/stat.h>

#include "theme_model.h"

ThemeModel::ThemeModel() {
    load_data();
}

App::ModelItemInfo Theme::info(int field, int request) const {
    auto info = App::ModelItemInfo {};
    switch (field) {
        case ThemeModel::Column::Name:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text(palette()->name());
            break;
        default:
            break;
    }
    return info;
}

App::ModelItemInfo ThemeModel::header_info(int field, int request) const {
    auto info = App::ModelItemInfo {};
    switch (field) {
        case Column::Name:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text("Name");
            break;
        default:
            break;
    }
    return info;
}

void ThemeModel::load_data() {
    auto* root_item = model_item_root();
    root_item->clear_children();

    dirent** dirents;
    int dirent_count;
    if ((dirent_count = scandir(RESOURCE_ROOT "/usr/share/themes", &dirents, nullptr, nullptr)) == -1) {
        return;
    }

    for (int i = 0; i < dirent_count; i++) {
        auto* dirent = dirents[i];
        if (StringView(dirent->d_name) == StringView(".") || StringView(dirent->d_name) == StringView("..")) {
            continue;
        }

        auto path = String::format(RESOURCE_ROOT "/usr/share/themes/%s", dirent->d_name);
        auto theme = Palette::create_from_json(path);
        if (theme) {
            root_item->add_child<Theme>(move(path), move(theme));
        }

        free(dirent);
    }
    free(dirents);
}
