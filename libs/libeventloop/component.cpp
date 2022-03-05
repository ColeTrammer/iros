#include <eventloop/component.h>
#include <eventloop/object.h>

namespace App {
Component::Component(Object& object) : m_object(object) {
    object.register_component(*this);
}
}
