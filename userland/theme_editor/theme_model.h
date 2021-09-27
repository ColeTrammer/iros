#pragma once

#include <app/model.h>
#include <graphics/palette.h>
#include <liim/vector.h>
#include <sys/types.h>

struct Theme {
    String path;
    SharedPtr<Palette> palette;
};

class ThemeModel final : public App::Model {
    APP_OBJECT(ThemeModel)

public:
    ThemeModel();

    enum Column {
        Name,
        __Count,
    };

    virtual ModelDimensions dimensions() const override { return { .item_count = m_themes.size(), .field_count = Column::__Count }; }
    virtual App::ModelItemInfo item_info(const App::ModelIndex& index, int request) const override;
    virtual App::ModelItemInfo header_info(int field, int request) const override;

    const Vector<Theme>& themes() const { return m_themes; }

private:
    void load_data();

    Vector<Theme> m_themes;
};
