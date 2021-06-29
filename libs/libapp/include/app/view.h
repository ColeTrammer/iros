#pragma once

#include <app/model_client.h>
#include <app/model_index.h>
#include <app/widget.h>
#include <liim/hash_set.h>

namespace App {

class Model;

class Selection {
public:
    bool present(const ModelIndex& index) const { return !!m_indexes.get(index); }
    void toggle(const ModelIndex& index) { m_indexes.toggle(index); }
    void add(ModelIndex index) { m_indexes.put(move(index)); }
    void remove(const ModelIndex& index) { m_indexes.remove(index); }

    void clear() { m_indexes.clear(); }
    bool empty() const { return m_indexes.size() == 0; }

    const ModelIndex& first() const {
        ModelIndex* ret = nullptr;
        m_indexes.for_each([&](auto& i) {
            ret = &i;
        });
        return *ret;
    }

private:
    HashSet<ModelIndex> m_indexes;
};

class View
    : public Widget
    , public ModelClient {
    APP_OBJECT(View)

public:
    virtual ~View() override;

    Model* model() { return m_model.get(); }
    const Model* model() const { return m_model.get(); }

    void set_model(SharedPtr<Model> model);

    const ModelIndex& hovered_index() const { return m_hovered_index; }
    void set_hovered_index(ModelIndex);

    virtual void on_mouse_down(const MouseEvent&) override;
    virtual void on_mouse_move(const MouseEvent&) override;
    virtual void on_leave() override { set_hovered_index({}); }

    virtual void model_did_update() override { invalidate(); }

    const Selection& selection() const { return m_selection; }

    void add_to_selection(const ModelIndex& index) { m_selection.add(index); }
    void toggle_selection(const ModelIndex& index) { m_selection.toggle(index); }
    void remove_from_selection(const ModelIndex& index) { m_selection.remove(index); }
    bool is_selected(const ModelIndex& index) const { return m_selection.present(index); }
    void clear_selection() { m_selection.clear(); }
    bool has_selection() const { return !m_selection.empty(); }

protected:
    virtual ModelIndex index_at_position(int wx, int wy) = 0;

private:
    SharedPtr<Model> m_model;
    ModelIndex m_hovered_index;
    Selection m_selection;
};
}
