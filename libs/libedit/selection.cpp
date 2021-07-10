#include <edit/selection.h>

namespace Edit {
bool Selection::overlaps(const Selection& other) const {
    if (this->empty() && other.empty()) {
        return this->start() == other.start();
    }

    auto this_start = this->normalized_start();
    auto this_end = this->normalized_end();

    auto other_start = other.normalized_start();
    auto other_end = other.normalized_end();

    // FIXME: although correct, there certainly exists a simpler algorithm.
    return (this_start >= other_start && this_start < other_end) || (this_end > other_start && this_end <= other_end) ||
           (other_start >= this_start && other_start < this_end) || (other_end > this_start && other_end <= this_end);
}

void Selection::merge(const Selection& other) {
    auto this_start = this->normalized_start();
    auto this_end = this->normalized_end();

    auto other_start = other.normalized_start();
    auto other_end = other.normalized_end();

    auto new_start = min(this_start, other_start);
    auto new_end = max(this_end, other_end);

    if (this->start() <= this->end()) {
        set(new_start, new_end);
    } else {
        set(new_end, new_start);
    }
}
}
