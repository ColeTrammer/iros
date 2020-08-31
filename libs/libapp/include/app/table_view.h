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

protected:
    int width_of(const ModelData& data) const;
    void render_data(Renderer& renderer, int rx, int ry, const ModelData& data);
};

}
