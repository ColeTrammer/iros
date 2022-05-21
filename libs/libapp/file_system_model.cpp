#include <app/file_system_model.h>
#include <app/tree_view.h>
#include <dirent.h>
#include <graphics/png.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>

// #define FILE_SYSTEM_MODEL_DEBUG

namespace App {
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

ModelItemInfo FileSystemObject::info(int field, int request) const {
    auto info = ModelItemInfo {};
    switch (field) {
        case FileSystemModel::Column::Name:
            if (request & ModelItemInfo::Request::Text)
                info.set_text(name());
            if (request & ModelItemInfo::Request::Bitmap)
                info.set_bitmap(icon());
            break;
        case FileSystemModel::Column::Owner:
            if (request & ModelItemInfo::Request::Text)
                info.set_text(owner());
            break;
        case FileSystemModel::Column::Group:
            if (request & ModelItemInfo::Request::Text)
                info.set_text(group());
            break;
        case FileSystemModel::Column::Size:
            if (request & ModelItemInfo::Request::Text)
                info.set_text(format("{}", size()));
            break;
        default:
            break;
    }
    return info;
}

bool FileSystemObject::openable() const {
    return S_ISDIR(m_mode);
}

ModelItemInfo FileSystemModel::header_info(int field, int request) const {
    auto info = ModelItemInfo {};
    switch (field) {
        case Column::Name:
            if (request & ModelItemInfo::Request::Text)
                info.set_text("Name");
            break;
        case Column::Owner:
            if (request & ModelItemInfo::Request::Text)
                info.set_text("Owner");
            break;
        case Column::Group:
            if (request & ModelItemInfo::Request::Text)
                info.set_text("Group");
            break;
        case Column::Size:
            if (request & ModelItemInfo::Request::Text)
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

    auto& path = maybe_path.value();

    set_root(make_unique<FileSystemObject>(m_text_file_icon, "/", "root", "root", 0666, 4096));
    auto* object = typed_root<FileSystemObject>();
    for (auto part : path.components()) {
        load_data(*object);

        for (int i = 0; i < object->item_count(); i++) {
            auto& child = object->typed_item<FileSystemObject>(i);
            if (child.name() == part) {
                object = &child;
                break;
            }
        }
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

#ifdef FILE_SYSTEM_MODEL_DEBUG
    error_log("Loading data for: `{}'", base_path);
#endif /* FILE_SYSTEM_MODEL_DEBUG */

    dirent** dirents;
    int dirent_count;
    if ((dirent_count = scandir(base_path.to_string().string(), &dirents, ignore_dots, &alphasort)) == -1) {
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

void FileSystemModel::install_on_tree_view(TreeView& view) {
    listen<TreeViewItemExpanded>(view, [this, &view](const TreeViewItemExpanded& event) {
        if (view.model() == this) {
            load_data(static_cast<FileSystemObject&>(*event.item()));
        }
    });
}
}
