#pragma once

#include <app/model.h>
#include <graphics/palette.h>
#include <liim/vector.h>
#include <sys/types.h>

class Theme : public App::ModelItem {
public:
    Theme(String path, SharedPtr<Palette> palette) : m_path(move(path)), m_palette(move(palette)) {}

    virtual App::ModelItemInfo info(int field, int request) const override;

    const String& path() const { return m_path; }
    SharedPtr<Palette> palette() const { return m_palette; }

private:
    String m_path;
    SharedPtr<Palette> m_palette;
};

class ThemeModel final : public App::Model {
    APP_OBJECT(ThemeModel)

public:
    ThemeModel();

    enum Column {
        Name,
        __Count,
    };

    virtual int field_count() const override { return Column::__Count; }
    virtual App::ModelItemInfo header_info(int field, int request) const override;

private:
    void load_data();
};
