#include <app/base/view.h>
#include <app/base/widget.h>
#include <app/model.h>

namespace App::Base {
View::View() {}

void View::initialize() {
    this_widget().set_accepts_focus(true);

    this_widget().on<MouseDownEvent>({}, [this](const MouseDownEvent& event) {
        auto index = index_at_position(event.x(), event.y());
        if (event.left_button()) {
            switch (event.cyclic_count(2)) {
                case 1:
                    selection().clear();
                    if (index.valid()) {
                        selection().add(index);
                    }
                    invalidate_all();
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

    this_widget().on<MouseMoveEvent>({}, [this](const MouseMoveEvent& event) {
        auto index = index_at_position(event.x(), event.y());
        set_hovered_index(index);
        return false;
    });

    this_widget().on<LeaveEvent>({}, [this](const LeaveEvent&) {
        set_hovered_index({});
    });
}

View::~View() {}

void View::set_model(SharedPtr<Model> model) {
    if (m_model == model) {
        return;
    }

    if (m_model) {
        uninstall_model_listeners(*m_model);
        set_root_item(nullptr);
    }

    m_model = model;
    if (m_model) {
        install_model_listeners(*m_model);
        set_root_item(m_model->model_item_root());
    }

    m_hovered_index.clear();
    invalidate_all();
}

void View::set_hovered_index(ModelIndex index) {
    if (index == m_hovered_index) {
        return;
    }

    m_hovered_index = move(index);
    invalidate_all();
}

void View::install_model_listeners(Model& model) {
    model.on<ModelUpdateEvent>(this_widget(), [this](auto&) {
        if (!root_item()) {
            set_root_item(this->model()->model_item_root());
        }
        invalidate_all();
    });
}

void View::uninstall_model_listeners(Model& model) {
    model.remove_listener(this_widget());
}

void View::set_root_item(ModelItem* item) {
    if (m_root_item == item) {
        return;
    }

    m_root_item = item;
    this_widget().emit<ViewRootChanged>();
}
}
