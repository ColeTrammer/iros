#include <dirent.h>
#include <graphics/png.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>

#include "file_system_model.h"

FileSystemModel::FileSystemModel() {
    m_text_file_icon = decode_png_file("/usr/share/text-file-32.png");
    assert(m_text_file_icon);
}

FileSystemModel::~FileSystemModel() {}

const FileSystemObject& FileSystemModel::object_from_index(const App::ModelIndex& index) {
    return m_objects[index.row()];
}

String FileSystemModel::full_path(const String& name) {
    return m_base_path.join_component(name);
}

void FileSystemModel::set_base_path(String path_string) {
    auto new_path = Ext::Path::resolve(path_string);
    if (!new_path) {
        return;
    }

    m_base_path = move(*new_path);
    load_data();
}

void FileSystemModel::go_to_parent() {
    m_base_path.set_to_parent();
    load_data();
}

App::ModelData FileSystemModel::data(const App::ModelIndex& index, int role) const {
    int row = index.row();
    if (row < 0 || row >= m_objects.size()) {
        return {};
    }

    auto& object = m_objects[row];
    if (role == Role::Display) {
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

    if (role == Role::Icon) {
        return m_text_file_icon;
    }

    if (role == Role::TextAlignment) {
        switch (index.col()) {
            case Column::Icon:
                return {};
            case Column::Name:
            case Column::Owner:
            case Column::Group:
                return TextAlign::CenterLeft;
            case Column::Size:
                return TextAlign::CenterRight;
            default:
                return {};
        }
    }

    return {};
}

App::ModelData FileSystemModel::header_data(int col, int role) const {
    if (role == Role::Display) {
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
    return {};
}

static int ignore_dots(const dirent* a) {
    return a->d_name[0] != '.';
}

void FileSystemModel::load_data() {
    fprintf(stderr, "Load data for: `%s'\n", m_base_path.to_string().string());

    m_objects.clear();

    dirent** dirents;
    int dirent_count;
    if ((dirent_count = scandir(m_base_path.to_string().string(), &dirents, ignore_dots, nullptr)) == -1) {
        return;
    }

    for (int i = 0; i < dirent_count; i++) {
        auto* dirent = dirents[i];
        auto path = full_path(dirent->d_name);
        struct stat st;
        if (lstat(path.string(), &st) == 0) {
            passwd* pwd = getpwuid(st.st_uid);
            group* grp = getgrgid(st.st_gid);
            m_objects.add({ dirent->d_name, pwd ? pwd->pw_name : "Unknown", grp ? grp->gr_name : "Unknown", st.st_mode, st.st_size });
        }
        free(dirent);
    }
    free(dirents);

    did_update();
}
