#pragma once

#include <app/forward.h>
#include <app/selection.h>
#include <eventloop/event.h>
#include <liim/function.h>

APP_EVENT(App, ViewRootChanged, Event, (), (), ());
APP_EVENT(App, ViewItemActivated, Event, (), ((ModelItem*, item)), ())

namespace App::Base {
class View {
public:
    void initialize();
    ~View();

    Model* model() { return m_model.get(); }
    const Model* model() const { return m_model.get(); }

    void set_model(SharedPtr<Model> model);

    ModelItem* hovered_item() const { return m_hovered_item; }
    void set_hovered_item(ModelItem*);

    Selection& selection() { return m_selection; }
    const Selection& selection() const { return m_selection; }

    ModelItem* root_item() { return m_root_item; }
    const ModelItem* root_item() const { return m_root_item; }

    void set_root_item(ModelItem* item);

protected:
    View();

    virtual Base::Widget& this_widget() = 0;
    virtual void invalidate_all() = 0;
    virtual ModelItem* item_at_position(int wx, int wy) = 0;

    virtual void install_model_listeners(Model& model);
    virtual void uninstall_model_listeners(Model& model);

private:
    SharedPtr<Model> m_model;
    ModelItem* m_hovered_item;
    Selection m_selection;
    ModelItem* m_root_item { nullptr };
};
}
