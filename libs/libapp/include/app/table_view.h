#pragma once

#include <app/view.h>
#include <graphics/forward.h>
#include <liim/function.h>

namespace App {
class TableView : public View {
    APP_OBJECT(TableView)

public:
    virtual ~TableView() override;

    virtual void render() override;

    int cell_padding() const { return m_cell_padding; }
    void set_cell_padding(int p) { m_cell_padding = p; }

protected:
    int width_of(const ModelItemInfo& data) const;
    void render_data(Renderer& renderer, int rx, int ry, int width, Function<ModelItemInfo()> getter);

    virtual ModelIndex index_at_position(int, int) override;

private:
    int m_cell_padding { 2 };
};
}
