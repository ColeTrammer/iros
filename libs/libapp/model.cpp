#include <app/model.h>
#include <app/view.h>

namespace App {

void Model::register_view(View* view) {
    m_views.add(view);
}

void Model::unregister_view(View* view) {
    m_views.remove_element(view);
}

void Model::did_update() {
    for (auto* view : m_views) {
        view->invalidate();
    }
}

}
