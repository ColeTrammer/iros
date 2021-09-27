#pragma once

#include <app/forward.h>
#include <app/selection.h>
#include <liim/function.h>

namespace App::Base {
class View {
public:
    void initialize();
    ~View();

    Model* model() { return m_model.get(); }
    const Model* model() const { return m_model.get(); }

    void set_model(SharedPtr<Model> model);

    const ModelIndex& hovered_index() const { return m_hovered_index; }
    void set_hovered_index(ModelIndex);

    Selection& selection() { return m_selection; }
    const Selection& selection() const { return m_selection; }

    Function<void(const ModelIndex&)> on_item_activation;

protected:
    View();

    virtual Base::Widget& this_widget() = 0;
    virtual void invalidate_all() = 0;
    virtual ModelIndex index_at_position(int wx, int wy) = 0;

    virtual void install_model_listeners(Model& model);
    virtual void uninstall_model_listeners(Model& model);

private:
    SharedPtr<Model> m_model;
    ModelIndex m_hovered_index;
    Selection m_selection;
};
}
