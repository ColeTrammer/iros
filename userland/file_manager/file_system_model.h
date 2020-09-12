#pragma once

#include <app/model.h>
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
    FileSystemModel(String base_path);

    enum Column {
        Name,
        Owner,
        Group,
        Size,
        __Count,
    };

    virtual int row_count() const override { return m_objects.size(); }
    virtual int col_count() const override { return Column::__Count; }
    virtual App::ModelData data(const App::ModelIndex& index) const override;
    virtual App::ModelData header_data(int col) const override;

    const String& base_path() const { return m_base_path; }

private:
    void load_data();

    String m_base_path;
    Vector<FileSystemObject> m_objects;
};
