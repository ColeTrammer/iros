#pragma once

namespace di::sync {
class DumbSpinlock;
}
namespace dius {
using Mutex = di::sync::DumbSpinlock;
}
