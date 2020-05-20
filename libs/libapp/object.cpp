#include <app/object.h>

namespace App {

void Object::add_child(SharedPtr<Object> child) {
    child->set_parent(this);
    m_children.add(move(child));
}

Object::Object() {}

Object::~Object() {}

}
