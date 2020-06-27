#include <graphics/rect_set.h>

void RectSet::add(const Rect& rect) {
    if (rect.width() == 0 || rect.height() == 0) {
        return;
    }

    Vector<Rect> fragments = Vector<Rect>::create_from_single_element(rect);

    auto add_nonintersecting_part_of_rect = [&](const auto& main, const auto& sub) {
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
    };

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
            add_nonintersecting_part_of_rect(fragment, intersection);
            fragments.unstable_remove(i - 1);
        }
    }
    m_rects.add(move(fragments));
}
