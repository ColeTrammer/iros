#pragma once

#include <app/model_data.h>
#include <app/widget.h>

class Renderer;

namespace App {

class Model;

class TableView : public Widget {
    APP_OBJECT(TableView)

public:
    virtual ~TableView() override;

    virtual void render() override;

    void set_model(SharedPtr<Model> model);

protected:
    int width_of(const ModelData& data) const;
    void render_data(Renderer& renderer, int rx, int ry, const ModelData& data);

private:
    SharedPtr<Model> m_model;
};

}
