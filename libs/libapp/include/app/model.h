#pragma once

#include <app/model_data.h>
#include <app/model_index.h>
#include <eventloop/object.h>
#include <liim/hash_set.h>
#include <liim/string.h>
#include <liim/variant.h>

namespace App {

class ModelClient;

class Selection {
public:
    bool present(const ModelIndex& index) const { return !!m_indexes.get(index); }
    void toggle(const ModelIndex& index) { m_indexes.toggle(index); }
    void add(ModelIndex index) { m_indexes.put(move(index)); }
    void remove(const ModelIndex& index) { m_indexes.remove(index); }

    void clear() { m_indexes.clear(); }
    bool empty() const { return m_indexes.size() == 0; }

private:
    HashSet<ModelIndex> m_indexes;
};

class Model : public Object {
    APP_OBJECT(Model)

public:
    enum Role {
        Display,
        TextAlignment,
        Icon,
    };

    virtual int row_count() const = 0;
    virtual int col_count() const = 0;
    virtual ModelData data(const ModelIndex& index, int role) const = 0;
    virtual ModelData header_data(int col, int role) const = 0;

    void register_client(ModelClient* view);
    void unregister_client(ModelClient* view);

    const Selection& selection() const { return m_selection; }

    void add_to_selection(const ModelIndex& index) { m_selection.add(index); }
    void toggle_selection(const ModelIndex& index) { m_selection.toggle(index); }
    void remove_from_selection(const ModelIndex& index) { m_selection.remove(index); }
    bool is_selected(const ModelIndex& index) const { return m_selection.present(index); }

protected:
    void did_update();

private:
    Vector<ModelClient*> m_clients;
    Selection m_selection;
};

}
