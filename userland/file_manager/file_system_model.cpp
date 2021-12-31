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

Ext::Path FileSystemModel::full_path(const FileSystemObject& object) {
    return m_base_path.join_component(object.name());
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

App::ModelItemInfo FileSystemObject::info(int field, int request) const {
    auto info = App::ModelItemInfo {};
    switch (field) {
        case FileSystemModel::Column::Name:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text(name());
            if (request & App::ModelItemInfo::Request::Bitmap)
                info.set_bitmap(icon());
            break;
        case FileSystemModel::Column::Owner:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text(owner());
            break;
        case FileSystemModel::Column::Group:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text(group());
            break;
        case FileSystemModel::Column::Size:
            if (request & App::ModelItemInfo::Request::Text)
                info.set_text(format("{}", size()));
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

    auto* root_item = model_item_root();
    root_item->clear_children();

    dirent** dirents;
    int dirent_count;
    if ((dirent_count = scandir(m_base_path.to_string().string(), &dirents, ignore_dots, nullptr)) == -1) {
        return;
    }

    for (int i = 0; i < dirent_count; i++) {
        auto* dirent = dirents[i];
        auto path = m_base_path.join_component(dirent->d_name);
        struct stat st;
        if (lstat(path.to_string().string(), &st) == 0) {
            passwd* pwd = getpwuid(st.st_uid);
            group* grp = getgrgid(st.st_gid);
            root_item->add_child<FileSystemObject>(m_text_file_icon, dirent->d_name, pwd ? pwd->pw_name : "Unknown",
                                                   grp ? grp->gr_name : "Unknown", st.st_mode, st.st_size);
        }
        free(dirent);
    }
    free(dirents);

    did_update();
}
