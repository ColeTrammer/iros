#pragma once

namespace di::sync {
class NeverStopToken {
private:
    struct Callback {
        explicit Callback(NeverStopToken, auto&&) {}
    };

public:
    template<typename>
    using CallbackType = Callback;

    constexpr static auto stop_requested() -> bool { return false; }
    constexpr static auto stop_possible() -> bool { return false; }
};
}
