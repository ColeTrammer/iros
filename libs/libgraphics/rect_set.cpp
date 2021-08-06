#include <graphics/rect_set.h>

bool RectSet::intersects(const Point& point) const {
    for (auto& rect : m_rects) {
        if (rect.intersects(point)) {
            return true;
        }
    }
    return false;
}

bool RectSet::intersects(const Rect& rect_in) const {
    for (auto& rect : m_rects) {
        if (rect.intersects(rect_in)) {
            return true;
        }
    }
    return false;
}

template<typename FragmentsType>
static void add_nonintersecting_part_of_rect(FragmentsType& fragments, const Rect& main, const Rect& sub) {
    if (main.x() != sub.x()) {
        fragments.add(Rect(main.x(), main.y(), sub.x() - main.x(), main.height()));
    }

    if (main.x() + main.width() != sub.x() + sub.width()) {
        fragments.add(Rect(sub.x() + sub.width(), main.y(), main.x() + main.width() - sub.x() - sub.width(), main.height()));
    }

    if (main.y() != sub.y()) {
        fragments.add(Rect(sub.x(), main.y(), sub.width(), sub.y() - main.y()));
    }

    if (main.y() + main.height() != sub.y() + sub.height()) {
        fragments.add(Rect(sub.x(), sub.y() + sub.height(), sub.width(), main.y() + main.height() - sub.y() - sub.height()));
    }
}

void RectSet::add(const Rect& rect) {
    if (rect.width() == 0 || rect.height() == 0) {
        return;
    }

    auto fragments = Vector<Rect>::create_from_single_element(rect);
    for (size_t j = m_rects.size(); j > 0; j--) {
        auto& present_rect = m_rects[j - 1];
        for (size_t i = fragments.size(); i > 0; i--) {
            auto& fragment = fragments[i - 1];
            auto intersection = present_rect.intersection_with(fragment);

            // No intersection, ignore.
            if (intersection == Rect()) {
                continue;
            }

            // The fragment is completely contained by the present rect, discard the fragment.
            if (intersection == fragment) {
                fragments.unstable_remove(i - 1);
                continue;
            }

            // The fragment completely contains the present rect, remove the present rect.
            if (intersection == present_rect) {
                m_rects.unstable_remove(j - 1);
                break;
            }

            // There is a small piece of fragment that intersects the present rect, chop up the fragment.
            add_nonintersecting_part_of_rect(fragments, fragment, intersection);
            fragments.unstable_remove(i - 1);
        }
    }
    m_rects.add(move(fragments));
}

void RectSet::subtract(const Rect& rect) {
    auto output = RectSet {};
    for (auto& old_rect : m_rects) {
        if (!old_rect.intersects(rect)) {
            output.add(old_rect);
            continue;
        }

        auto intersection = old_rect.intersection_with(rect);
        if (intersection == old_rect) {
            continue;
        }

        add_nonintersecting_part_of_rect(output, old_rect, intersection);
    }
    swap(output);
}
