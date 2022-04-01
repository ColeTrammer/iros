#include <app/model.h>

namespace App {
Model::Model() {
    set_root(make_unique<ModelItem>());
}

void Model::set_root(UniquePtr<ModelItem> root) {
    m_root = move(root);
    did_set_root(m_root.get());
}

void Model::did_insert_child(ModelItem* parent, ModelItem* child, int index) {
    emit<ModelDidInsertItem>(parent, child, index);
}

void Model::did_remove_child(ModelItem* parent, ModelItem* child_warning_stale, int index) {
    emit<ModelDidRemoveItem>(parent, child_warning_stale, index);
}

void Model::did_set_root(ModelItem* new_root) {
    emit<ModelDidSetRoot>(new_root);
}
}
