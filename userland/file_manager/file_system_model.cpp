#include <dirent.h>
#include <graphics/png.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>

#include "file_system_model.h"

FileSystemModel::FileSystemModel(String base_path) : m_base_path(move(base_path)) {
    m_text_file_icon = decode_png_file("/usr/share/text-file-16.png");
    assert(m_text_file_icon);
    load_data();
}

App::ModelData FileSystemModel::data(const App::ModelIndex& index) const {
    int row = index.row();
    if (row < 0 || row >= m_objects.size()) {
        return {};
    }

    auto& object = m_objects[row];
    switch (index.col()) {
        case Column::Icon:
            return m_text_file_icon;
        case Column::Name:
            return object.name;
        case Column::Owner:
            return object.owner;
        case Column::Group:
            return object.group;
        case Column::Size:
            return String::format("%lu", object.size);
        default:
            return {};
    }
}

App::ModelData FileSystemModel::header_data(int col) const {
    switch (col) {
        case Column::Icon:
            return {};
        case Column::Name:
            return "Name";
        case Column::Owner:
            return "Owner";
        case Column::Group:
            return "Group";
        case Column::Size:
            return "Size";
        default:
            return {};
    }
}

static int ignore_dots(const dirent* a) {
    return a->d_name[0] != '.';
}

void FileSystemModel::load_data() {
    m_objects.clear();

    dirent** dirents;
    int dirent_count;
    if ((dirent_count = scandir(m_base_path.string(), &dirents, ignore_dots, nullptr)) == -1) {
        return;
    }

    for (int i = 0; i < dirent_count; i++) {
        auto* dirent = dirents[i];
        auto path = String::format("%s%s", m_base_path.string(), dirent->d_name);
        struct stat st;
        if (lstat(path.string(), &st) == 0) {
            passwd* pwd = getpwuid(st.st_uid);
            group* grp = getgrgid(st.st_gid);
            m_objects.add({ dirent->d_name, pwd ? pwd->pw_name : "Unknown", grp ? grp->gr_name : "Unknown", st.st_mode, st.st_size });
        }
        free(dirent);
    }
    free(dirents);
}
