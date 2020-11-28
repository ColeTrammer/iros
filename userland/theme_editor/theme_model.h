#pragma once

#include <app/model.h>
#include <liim/vector.h>
#include <sys/types.h>

struct Theme {
    String name;
};

class ThemeModel final : public App::Model {
    APP_OBJECT(ThemeModel)

public:
    ThemeModel();

    enum Column {
        Name,
        __Count,
    };

    virtual int row_count() const override { return m_themes.size(); }
    virtual int col_count() const override { return Column::__Count; }
    virtual App::ModelData data(const App::ModelIndex& index, int role) const override;
    virtual App::ModelData header_data(int col, int role) const override;

private:
    void load_data();

    Vector<Theme> m_themes;
};
