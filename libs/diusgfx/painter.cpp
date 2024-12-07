#include "diusgfx/painter.h"

#include "di/function/tag_invoke.h"
#include "di/math/abs_diff.h"
#include "diusgfx/bitmap.h"

namespace gfx {
class SimplePainter {
public:
    explicit SimplePainter(ExclusiveBitMap bitmap) : m_bitmap(di::move(bitmap)) {}

private:
    friend void tag_invoke(di::Tag<draw_rect>, SimplePainter& self, Rect rect, Color color) {
        auto argb = color.as_rgba32();
        auto pixel_value = ARGBPixel { argb.b, argb.g, argb.r, argb.a };

        auto sx = usize(rect.x());
        auto sy = usize(rect.y());
        auto width = usize(rect.width());
        auto height = usize(rect.height());

        for (auto y = sy; y < sy + height && y < self.m_bitmap.height(); y++) {
            for (auto x = sx; x < sx + width && x < self.m_bitmap.width(); x++) {
                self.m_bitmap.argb_pixels()[y, x] = pixel_value;
            }
        }
    }

    friend void tag_invoke(di::Tag<draw_circle>, SimplePainter& self, Point center, float radius, Color color) {
        auto argb = color.as_rgba32();
        auto pixel_value = ARGBPixel { argb.b, argb.g, argb.r, argb.a };

        auto cx = usize(center.x());
        auto cy = usize(center.y());
        auto r = usize(radius);

        auto sx = r > cx ? 0 : cx - r;
        auto sy = r > cy ? 0 : cy - r;

        for (auto y = sy; y <= cy + r && y < self.m_bitmap.height(); y++) {
            for (auto x = sx; x <= cx + r && x < self.m_bitmap.width(); x++) {
                auto dx = di::abs_diff(x, cx);
                auto dy = di::abs_diff(y, cy);
                if (dx * dx + dy * dy < r * r) {
                    self.m_bitmap.argb_pixels()[y, x] = pixel_value;
                }
            }
        }
    }

    ExclusiveBitMap m_bitmap;
};

auto make_painter(ExclusiveBitMap bitmap) -> Painter {
    return SimplePainter(di::move(bitmap));
}
}
