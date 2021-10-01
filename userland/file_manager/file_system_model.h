#pragma once

#include <app/model.h>
#include <ext/path.h>
#include <liim/vector.h>
#include <sys/types.h>

class FileSystemObject : public App::ModelItem {
public:
    FileSystemObject(SharedPtr<Bitmap> icon, String name, String owner, String group, mode_t mode, off_t size)
        : m_icon(move(icon)), m_name(move(name)), m_owner(move(owner)), m_group(move(group)), m_mode(mode), m_size(size) {}

    virtual App::ModelItemInfo info(int field, int request) const override;

    SharedPtr<Bitmap> icon() const { return m_icon; }
    const String& name() const { return m_name; }
    const String& owner() const { return m_owner; }
    const String& group() const { return m_group; }
    mode_t mode() const { return m_mode; }
    off_t size() const { return m_size; }

private:
    SharedPtr<Bitmap> m_icon;
    String m_name;
    String m_owner;
    String m_group;
    mode_t m_mode;
    off_t m_size;
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

    virtual int field_count() const override { return Column::__Count; }
    virtual App::ModelItemInfo header_info(int field, int request) const override;

    void set_base_path(String path);
    void go_to_parent();

    String full_path(const String& name);

private:
    void load_data();

    Ext::Path m_base_path;
    SharedPtr<Bitmap> m_text_file_icon;
};
