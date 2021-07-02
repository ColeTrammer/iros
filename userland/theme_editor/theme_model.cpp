#include <dirent.h>
#include <sys/stat.h>

#include "theme_model.h"

ThemeModel::ThemeModel() {
    load_data();
}

App::ModelData ThemeModel::data(const App::ModelIndex& index, int role) const {
    int row = index.row();
    if (row < 0 || row >= m_themes.size()) {
        return {};
    }

    auto& theme = m_themes[row];
    if (role == Role::Display) {
        switch (index.col()) {
            case Column::Name:
                return theme.palette->name();
            default:
                return {};
        }
    }

    if (role == Role::TextAlignment) {
        switch (index.col()) {
            case Column::Name:
                return TextAlign::CenterLeft;
            default:
                return {};
        }
    }

    return {};
}

App::ModelData ThemeModel::header_data(int col, int role) const {
    if (role == Role::Display) {
        switch (col) {
            case Column::Name:
                return "Name";
            default:
                return {};
        }
    }
    return {};
}

void ThemeModel::load_data() {
    m_themes.clear();

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
            m_themes.add(Theme { move(path), move(theme) });
        }

        free(dirent);
    }
    free(dirents);
}
