#pragma once

#include <app/model_data.h>
#include <app/view.h>

class Renderer;

namespace App {

class TableView : public View {
    APP_OBJECT(TableView)

public:
    virtual ~TableView() override;

    virtual void render() override;

    int cell_padding() const { return m_cell_padding; }
    void set_cell_padding(int p) { m_cell_padding = p; }

protected:
    int width_of(const ModelData& data) const;
    void render_data(Renderer& renderer, int rx, int ry, const ModelData& data);

private:
    int m_cell_padding { 2 };
};

}
