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

    constexpr static bool stop_requested() { return false; }
    constexpr static bool stop_possible() { return false; }
};
}
