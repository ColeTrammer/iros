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
    if (!object.parent()) {
        return Ext::Path::root();
    }

    auto base = full_path(*object.typed_parent<FileSystemObject>());
    return base.join_component(object.name());
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

FileSystemObject* FileSystemModel::load_initial_data(const String& string_path) {
    auto maybe_path = Ext::Path::resolve(string_path);
    if (!maybe_path) {
        return nullptr;
    }

    auto& path = *maybe_path;
    auto* object = typed_root<FileSystemObject>();
    for (auto part : path.components()) {
        object = &add_child<FileSystemObject>(*object, nullptr, part, "", "", 0, 0);
    }
    load_data(*object);
    return object;
}

void FileSystemModel::load_data(FileSystemObject& object) {
    if (object.loaded()) {
        return;
    }
    clear_children(object);
    object.set_loaded(true);

    auto base_path = full_path(object);
    error_log("Loading data for: `{}'", base_path);

    dirent** dirents;
    int dirent_count;
    if ((dirent_count = scandir(base_path.to_string().string(), &dirents, ignore_dots, nullptr)) == -1) {
        return;
    }

    for (int i = 0; i < dirent_count; i++) {
        auto* dirent = dirents[i];
        auto path = base_path.join_component(dirent->d_name);
        struct stat st;
        if (lstat(path.to_string().string(), &st) == 0) {
            passwd* pwd = getpwuid(st.st_uid);
            group* grp = getgrgid(st.st_gid);
            add_child<FileSystemObject>(object, m_text_file_icon, dirent->d_name, pwd ? pwd->pw_name : "Unknown",
                                        grp ? grp->gr_name : "Unknown", st.st_mode, st.st_size);
        }
        free(dirent);
    }
    free(dirents);
}
