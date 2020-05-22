#include <app/event_loop.h>
#include <app/selectable.h>
#include <liim/vector.h>

namespace App {

static Vector<Selectable*> s_selectables;

void EventLoop::register_selectable(Selectable& selectable) {
    s_selectables.add(&selectable);
}

void EventLoop::unregister_selectable(Selectable& selectable) {
    s_selectables.remove_element(&selectable);
}

}
