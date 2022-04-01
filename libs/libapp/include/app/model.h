#pragma once

#include <app/forward.h>
#include <app/model_item.h>
#include <app/model_item_info.h>
#include <eventloop/event.h>
#include <eventloop/object.h>

APP_EVENT(App, ModelDidInsertItem, Event, (), ((ModelItem*, parent), (ModelItem*, child), (int, index_into_parent)), ())
APP_EVENT(App, ModelDidRemoveItem, Event, (), ((ModelItem*, parent), (ModelItem*, child_warning_stale), (int, index_into_parent)), ())
APP_EVENT(App, ModelDidSetRoot, Event, (), ((ModelItem*, new_root)), ())

namespace App {
class Model : public Object {
    APP_OBJECT(Model)

    APP_EMITS(Object, ModelDidInsertItem, ModelDidRemoveItem, ModelDidSetRoot)

public:
    virtual int field_count() const = 0;
    virtual ModelItemInfo header_info(int field, int request) const = 0;

    ModelItem* model_item_root() { return m_root.get(); }
    const ModelItem* model_item_root() const { return m_root.get(); }

    template<typename T>
    T* typed_root() {
        return static_cast<T*>(m_root.get());
    }

    template<typename T>
    const T* typed_root() const {
        return static_cast<const T*>(m_root.get());
    }

    template<typename T, typename... Args>
    T& add_child(ModelItem& parent, Args&&... args) {
        auto child = make_unique<T>(forward<Args>(args)...);
        auto& ret = *child;
        parent.add_child(move(child));
        did_insert_child(&parent, &ret, parent.item_count() - 1);
        return ret;
    }

    template<typename T, typename... Args>
    T& insert_child(ModelItem& parent, int index, Args&&... args) {
        auto child = make_unique<T>(forward<Args>(args)...);
        auto& ret = *child;
        parent.insert_child(index, move(child));
        did_insert_child(&parent, &ret, index);
        return ret;
    }

    void remove_child(ModelItem& parent, int index) {
        auto* child = parent.model_item_at(index);
        parent.remove_child(index);
        did_remove_child(&parent, child, index);
    }

    void clear_children(ModelItem& parent) {
        for (int i = parent.item_count() - 1; i >= 0; i--) {
            remove_child(parent, i);
        }
    }

protected:
    Model();

    void did_insert_child(ModelItem* parent, ModelItem* child, int index);
    void did_remove_child(ModelItem* parent, ModelItem* child_warning_stale, int index);
    void did_set_root(ModelItem* new_root);

    void set_root(UniquePtr<ModelItem> root);

private:
    UniquePtr<ModelItem> m_root;
};
}
