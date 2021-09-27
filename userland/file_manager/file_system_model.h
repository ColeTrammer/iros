#pragma once

#include <app/model.h>
#include <ext/path.h>
#include <liim/vector.h>
#include <sys/types.h>

struct FileSystemObject {
    String name;
    String owner;
    String group;
    mode_t mode;
    off_t size;
};

class FileSystemModel final : public App::Model {
    APP_OBJECT(FileSystemModel)

public:
    FileSystemModel();
    virtual ~FileSystemModel() override;

    enum Column {
        Name,
        Owner,
        Group,
        Size,
        __Count,
    };

    virtual ModelDimensions dimensions() const override { return { .item_count = m_objects.size(), .field_count = Column::__Count }; }
    virtual App::ModelItemInfo item_info(const App::ModelIndex& index, int request) const override;
    virtual App::ModelItemInfo header_info(int field, int request) const override;

    void set_base_path(String path);
    void go_to_parent();

    String full_path(const String& name);

    const FileSystemObject& object_from_index(const App::ModelIndex& index);

private:
    void load_data();

    Ext::Path m_base_path;
    Vector<FileSystemObject> m_objects;
    SharedPtr<Bitmap> m_text_file_icon;
};
