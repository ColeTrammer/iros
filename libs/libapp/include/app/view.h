#pragma once

#include <app/model_index.h>
#include <app/widget.h>

namespace App {

class Model;

class View : public Widget {
    APP_OBJECT(View)

public:
    Model* model() { return m_model.get(); }
    const Model* model() const { return m_model.get(); }

    void set_model(SharedPtr<Model> model);

    const ModelIndex& hovered_index() const { return m_hovered_index; }
    void set_hovered_index(ModelIndex);

    virtual void on_mouse_event(MouseEvent&) override;
    virtual void on_leave() override { set_hovered_index({}); }

protected:
    virtual ModelIndex index_at_position(int wx, int wy) = 0;

private:
    SharedPtr<Model> m_model;
    ModelIndex m_hovered_index;
};

}
