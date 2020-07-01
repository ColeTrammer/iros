#include <app/object.h>

namespace App {

void Object::add_child(SharedPtr<Object> child) {
    child->set_parent(this);
    m_children.add(move(child));
}

void Object::remove_child(SharedPtr<Object> child) {
    child->set_parent(nullptr);
    m_children.remove_element(child);
}

Object::Object() {}

Object::~Object() {}

}
