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
        Icon,
        Name,
        Owner,
        Group,
        Size,
        __Count,
    };

    virtual int row_count() const override { return m_objects.size(); }
    virtual int col_count() const override { return Column::__Count; }
    virtual App::ModelData data(const App::ModelIndex& index, int role) const override;
    virtual App::ModelData header_data(int col, int role) const override;

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
