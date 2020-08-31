#pragma once

#include <app/widget.h>

namespace App {

class Model;

class View : public Widget {
    APP_OBJECT(View)

public:
    Model* model() { return m_model.get(); }
    const Model* model() const { return m_model.get(); }

    void set_model(SharedPtr<Model> model);

private:
    SharedPtr<Model> m_model;
};

}
