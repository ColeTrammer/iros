#include <app/model.h>

namespace App {
Model::Model() {
    set_root(make_unique<ModelItem>());
}

void Model::set_root(UniquePtr<ModelItem> root) {
    m_root = move(root);
}

void Model::did_update() {
    emit<ModelUpdateEvent>();
}
}
