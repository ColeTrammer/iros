#pragma once

#include <app/forward.h>
#include <app/model_item_info.h>
#include <liim/pointers.h>
#include <liim/vector.h>

namespace App {
class ModelItem {
public:
    ModelItem() = default;
    virtual ~ModelItem() = default;

    virtual ModelItemInfo info(int, int) const { return {}; }
    virtual bool openable() const { return item_count() > 0; }

    int item_count() const { return m_children.size(); }

    ModelItem* parent() { return m_parent; }
    const ModelItem* parent() const { return m_parent; }
    ModelItem* model_item_at(int index) { return m_children[index].get(); }
    const ModelItem* model_item_at(int index) const { return m_children[index].get(); }

    template<typename T>
    T& typed_item(int index) {
        return static_cast<T&>(*m_children[index].get());
    }

    template<typename T>
    const T& typed_item(int index) const {
        return static_cast<const T&>(*m_children[index].get());
    }

    template<typename T>
    T* typed_parent() {
        return static_cast<T*>(m_parent);
    }

    template<typename T>
    const T* typed_parent() const {
        return static_cast<const T*>(m_parent);
    }

    void add_child(UniquePtr<ModelItem> child) {
        child->set_parent(this);
        m_children.add(move(child));
    }

    void insert_child(int index, UniquePtr<ModelItem> child) {
        child->set_parent(this);
        m_children.insert(move(child), index);
    }

    void remove_child(int index) { m_children.remove(index); }
    void clear_children() { m_children.clear(); }

private:
    void set_parent(ModelItem* parent) { m_parent = parent; }

    Vector<UniquePtr<ModelItem>> m_children;
    ModelItem* m_parent { nullptr };
};
}
