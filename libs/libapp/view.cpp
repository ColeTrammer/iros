#include <app/event.h>
#include <app/model.h>
#include <app/view.h>

namespace App {

View::~View() {}

void View::set_model(SharedPtr<Model> model) {
    if (m_model == model) {
        return;
    }

    if (m_model) {
        m_model->unregister_client(this);
    }

    m_model = model;
    if (m_model) {
        m_model->register_client(this);
    }

    m_hovered_index.clear();
    invalidate();
}

void View::set_hovered_index(ModelIndex index) {
    if (index == m_hovered_index) {
        return;
    }

    m_hovered_index = move(index);
    invalidate();
}

void View::on_mouse_event(MouseEvent& event) {
    if (event.left() == MOUSE_NO_CHANGE && event.right() == MOUSE_NO_CHANGE) {
        set_hovered_index(index_at_position(event.x(), event.y()));
        return;
    }

    return Widget::on_mouse_event(event);
}

}
