#include <edit/text_range.h>

namespace Edit {
bool TextRange::includes(const TextIndex& index) const {
    return !starts_after(index) && !ends_before(index);
}

bool TextRange::starts_after(const TextIndex& index) const {
    return start().is_after(index);
}

bool TextRange::ends_before(const TextIndex& index) const {
    return end().is_before(index);
}
}
