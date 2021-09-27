#include <dirent.h>
#include <graphics/png.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>

#include "file_system_model.h"

FileSystemModel::FileSystemModel() {
    m_text_file_icon = decode_png_file(RESOURCE_ROOT "/usr/share/text-file-32.png");
    assert(m_text_file_icon);
}

FileSystemModel::~FileSystemModel() {}

const FileSystemObject& FileSystemModel::object_from_index(const App::ModelIndex& index) {
    return m_objects[index.item()];
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

App::ModelItemInfo FileSystemModel::item_info(const App::ModelIndex& index, int request) const {
    int row = index.item();
    if (row < 0 || row >= m_objects.size()) {
        return {};
    }

    auto info = App::ModelItemInfo {};
    auto& object = m_objects[row];
    switch (index.field()) {
        case Column::Name:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text(object.name);
            if (request & App::ModelItemInfo::Request::Bitmap)
                info.set_bitmap(m_text_file_icon);
            break;
        case Column::Owner:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text(object.owner);
            break;
        case Column::Group:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text(object.group);
            break;
        case Column::Size:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text(format("{}", object.size));
            break;
        default:
            break;
    }
    return info;
}

App::ModelItemInfo FileSystemModel::header_info(int field, int request) const {
    auto info = App::ModelItemInfo {};
    switch (field) {
        case Column::Name:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text("Name");
            break;
        case Column::Owner:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text("Owner");
            break;
        case Column::Group:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text("Group");
            break;
        case Column::Size:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text("Size");
            break;
    }
    return info;
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
