#include <app/model.h>
#include <app/view.h>
#include <eventloop/event.h>

namespace App {
void View::initialize() {
    on<MouseDownEvent>([this](const MouseDownEvent& event) {
        auto index = index_at_position(event.x(), event.y());
        if (event.left_button()) {
            switch (event.cyclic_count(2)) {
                case 1:
                    clear_selection();
                    if (index.valid()) {
                        add_to_selection(index);
                    }
                    invalidate();
                    break;
                case 2:
                    if (index.valid()) {
                        on_item_activation.safe_call(index);
                    }
                    break;
            }

            return true;
        }
        return false;
    });

    on<MouseMoveEvent>([this](const MouseMoveEvent& event) {
        auto index = index_at_position(event.x(), event.y());
        set_hovered_index(index);
        return false;
    });

    on<LeaveEvent>([this](const LeaveEvent&) {
        set_hovered_index({});
    });

    Widget::initialize();
}

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
}
