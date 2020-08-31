#include <app/model.h>
#include <app/view.h>

namespace App {

void View::set_model(SharedPtr<Model> model) {
    if (m_model == model) {
        return;
    }

    if (m_model) {
        m_model->unregister_view(this);
    }

    m_model = model;
    if (m_model) {
        m_model->register_view(this);
    }

    invalidate();
}

}
