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

    int item_count() const { return m_children.size(); }

    ModelItem* parent() { return m_parent; }
    ModelItem* model_item_at(int index) { return m_children[index].get(); }

    template<typename T>
    T& typed_item(int index) {
        return static_cast<T&>(*m_children[index].get());
    }

    template<typename T>
    T* typed_parent() {
        return static_cast<T*>(m_parent);
    }

    template<typename T, typename... Args>
    T& add_child(Args&&... args) {
        auto child = make_unique<T>(forward<Args>(args)...);
        auto& ret = *child;
        child->set_parent(this);
        m_children.add(move(child));
        return ret;
    }

    template<typename T, typename... Args>
    T& insert_child(int index, Args&&... args) {
        auto child = make_unique<T>(forward<Args>(args)...);
        auto& ret = *child;
        child->set_parent(this);
        m_children.insert(move(child), index);
        return ret;
    }

    void remove_child(int index) { m_children.remove(index); }
    void clear_children() { m_children.clear(); }

private:
    void set_parent(ModelItem* parent) { m_parent = parent; }

    Vector<UniquePtr<ModelItem>> m_children;
    ModelItem* m_parent { nullptr };
};
}
