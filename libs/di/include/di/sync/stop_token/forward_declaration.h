#pragma once

namespace di::sync {
namespace detail {
    class InPlaceStopCallbackBase;
}

class InPlaceStopToken;
class InPlaceStopSource;
template<typename>
class InPlaceStopCallback;

class NeverStopToken;
}

namespace di {
using sync::InPlaceStopCallback;
using sync::InPlaceStopSource;
using sync::InPlaceStopToken;
using sync::NeverStopToken;
}
