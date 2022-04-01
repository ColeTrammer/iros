#pragma once

#include <gui/forward.h>
#include <gui/view.h>
#include <liim/function.h>

namespace GUI {
class TableView : public View {
    APP_WIDGET(View, TableView)

public:
    TableView() {}
    virtual ~TableView() override;

    virtual void render() override;

    int cell_padding() const { return m_cell_padding; }
    void set_cell_padding(int p) { m_cell_padding = p; }

protected:
    int width_of(const App::ModelItemInfo& data) const;
    void render_data(Renderer& renderer, int rx, int ry, int width, Function<App::ModelItemInfo()> getter);

    virtual App::ModelItem* item_at_position(const Point& point) override;

private:
    int m_cell_padding { 2 };
};
}
