#include <graphics/rect.h>
#include <test/test.h>

TEST(rect, basic) {
    auto rect = Rect { 1, 2, 3, 4 };
    auto rect2 = Rect { { 1, 2 }, { 4, 6 } };
    EXPECT(rect == Rect { 1, 2, 3, 4 });
    EXPECT(rect2 == rect);

    EXPECT(rect.x() == 1);
    EXPECT(rect.y() == 2);
    EXPECT(rect.width() == 3);
    EXPECT(rect.height() == 4);

    EXPECT(rect.left() == 1);
    EXPECT(rect.top() == 2);
    EXPECT(rect.right() == 4);
    EXPECT(rect.bottom() == 6);

    EXPECT(Rect {}.empty());
    EXPECT(!rect.empty());

    EXPECT(rect.top_left() == Point { 1, 2 });
    EXPECT(rect.top_right() == Point { 4, 2 });
    EXPECT(rect.bottom_left() == Point { 1, 6 });
    EXPECT(rect.bottom_right() == Point { 4, 6 });
    EXPECT(rect.center() == Point { 2, 4 });

    EXPECT(rect.top_edge() == Rect { 1, 2, 3, 1 });
    EXPECT(rect.right_edge() == Rect { 3, 2, 1, 4 });
    EXPECT(rect.bottom_edge() == Rect { 1, 5, 3, 1 });
    EXPECT(rect.left_edge() == Rect { 1, 2, 1, 4 });

    rect.set_x(0);
    rect.set_y(0);
    rect.set_width(0);
    rect.set_height(0);
    EXPECT(rect == Rect {});
}

TEST(rect, intersection) {
    EXPECT(Rect { 10, 10, 500, 500 }.intersects(Point { 15, 15 }));
    EXPECT(!Rect { 10, 10, 500, 500 }.intersects(Point { 510, 510 }));
    EXPECT(!Rect { 10, 10, 500, 500 }.intersects(Rect {}));
    EXPECT(!Rect { 10, 10, 500, 500 }.intersects(Rect { 550, 50, 5, 5 }));
    EXPECT(Rect { 10, 10, 500, 500 }.intersects(Rect { 50, 50, 50, 50 }));

    EXPECT(Rect { 50, 50, 50, 50 }.intersection_with(Rect { 60, 60, 10, 10 }) == Rect { 60, 60, 10, 10 });
    EXPECT(Rect { 100, 25, 50, 50 }.intersection_with(Rect { 50, 35, 100, 10 }) == Rect { 100, 35, 50, 10 });
    EXPECT(Rect { 0, 0, 500, 500 }.intersection_with(Rect { 50, 50, 500, 500 }) == Rect { 50, 50, 450, 450 });
    EXPECT(Rect { 50, 50, 50, 50 }.intersection_with(Rect { 10, 10, 10, 10 }) == Rect {});
    EXPECT(Rect { 25, 25, 450, 450 }.intersection_with(Rect { 50, 50, 300, 300 }) == Rect { 50, 50, 300, 300 });
    EXPECT(Rect { 40, 60, 10, 100 }.intersection_with(Rect { 25, 25, 450, 450 }) == Rect { 40, 60, 10, 100 });
}

TEST(rect, generators) {
    auto rect = Rect { 10, 10, 90, 90 };
    EXPECT(rect.adjusted(5) == Rect { 5, 5, 100, 100 });
    EXPECT(rect.translated({ 1, 1 }) == Rect { 11, 11, 90, 90 });
    EXPECT(rect.positioned({ 0, 0 }) == Rect { 0, 0, 90, 90 });
    EXPECT(rect.expanded(5) == Rect { 10, 10, 95, 95 });
    EXPECT(rect.shrinked(5) == Rect { 10, 10, 85, 85 });
    EXPECT(rect.with_x(0) == Rect { 0, 10, 90, 90 });
    EXPECT(rect.with_y(0) == Rect { 10, 0, 90, 90 });
    EXPECT(rect.with_width(5) == Rect { 10, 10, 5, 90 });
    EXPECT(rect.with_height(5) == Rect { 10, 10, 90, 5 });
}
