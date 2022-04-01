#include <app/model.h>
#include <app/view.h>
#include <app/view_bridge.h>
#include <app/widget.h>
#include <app/widget_bridge.h>

namespace App {
View::View(SharedPtr<WidgetBridge> widget_bridge, SharedPtr<ViewBridge> view_bridge,
           SharedPtr<ScrollComponentBridge> scroll_component_bridge)
    : Widget(move(widget_bridge)), ScrollComponent(*this, move(scroll_component_bridge)), m_bridge(move(view_bridge)) {}

void View::initialize() {
    set_accepts_focus(true);

    on<MouseDownEvent>([this](const MouseDownEvent& event) {
        auto item = item_at_position({ event.x(), event.y() });
        if (event.left_button()) {
            switch (event.cyclic_count(2)) {
                case 1:
                    selection().clear();
                    if (item) {
                        selection().add(*item);
                    }
                    bridge().invalidate_all();
                    break;
                case 2:
                    if (item) {
                        emit<ViewItemActivated>(item);
                    }
                    break;
            }

            return true;
        }
        return false;
    });

    on<MouseMoveEvent>({}, [this](const MouseMoveEvent& event) {
        auto item = item_at_position({ event.x(), event.y() });
        set_hovered_item(item);
        return false;
    });

    on<LeaveEvent>({}, [this](const LeaveEvent&) {
        set_hovered_item(nullptr);
    });

    ScrollComponent::initialize();
    Widget::initialize();
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

    m_hovered_item = nullptr;
    bridge().invalidate_all();
}

void View::set_hovered_item(ModelItem* item) {
    if (item == m_hovered_item) {
        return;
    }

    m_hovered_item = item;
    bridge().invalidate_all();
}

void View::install_model_listeners(Model& model) {
    listen<App::ModelDidInsertItem, App::ModelDidRemoveItem, App::ModelDidSetRoot>(model, [this](auto&) {
        if (!root_item()) {
            set_root_item(this->model()->model_item_root());
        }
        bridge().invalidate_all();
    });
}

void View::uninstall_model_listeners(Model& model) {
    model.remove_listener(*this);
}

void View::set_root_item(ModelItem* item) {
    if (m_root_item == item) {
        return;
    }

    m_root_item = item;
    emit<ViewRootChanged>();
}
}
