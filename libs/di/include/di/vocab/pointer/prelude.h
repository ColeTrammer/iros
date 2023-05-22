#pragma once

#include <di/vocab/pointer/arc.h>
#include <di/vocab/pointer/box.h>
#include <di/vocab/pointer/intrusive_ptr.h>
#include <di/vocab/pointer/rc.h>

namespace di {
using vocab::AdoptObject;
using vocab::Arc;
using vocab::Box;
using vocab::DefaultDelete;
using vocab::IntrusivePtr;
using vocab::IntrusiveRefCount;
using vocab::IntrusiveThreadUnsafeRefCount;
using vocab::Rc;
using vocab::RetainObject;

using vocab::adopt_object;
using vocab::make_arc;
using vocab::make_box;
using vocab::make_rc;
using vocab::retain_object;
}
