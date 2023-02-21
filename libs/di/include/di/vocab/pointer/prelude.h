#pragma once

#include <di/vocab/pointer/arc.h>
#include <di/vocab/pointer/box.h>
#include <di/vocab/pointer/intrusive_ptr.h>
#include <di/vocab/pointer/rc.h>

namespace di {
using vocab::Arc;
using vocab::Box;
using vocab::IntrusivePtr;
using vocab::IntrusiveRefCount;
using vocab::IntrusiveThreadUnsafeRefCount;
using vocab::Rc;

using vocab::make_arc;
using vocab::make_box;
using vocab::make_rc;
using vocab::try_box;
using vocab::try_make_arc;
using vocab::try_make_rc;
}
