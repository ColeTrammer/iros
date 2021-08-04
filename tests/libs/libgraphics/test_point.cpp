#include <graphics/point.h>
#include <test/test.h>

TEST(point, basic) {
    auto p = Point {};
    EXPECT(p.x() == 0);
    EXPECT(p.y() == 0);

    auto q = Point { -1, 5 };
    EXPECT(q.x() == -1);
    EXPECT(q.y() == 5);

    q.set_x(2);
    q.set_y(3);
    EXPECT(q.x() == 2);
    EXPECT(q.y() == 3);

    EXPECT(p != q);
    EXPECT(p == p);
    EXPECT(p == Point { 0, 0 });
    EXPECT(q == Point { 2, 3 });

    EXPECT(-q == Point { -2, -3 });
}

TEST(point, generators) {
    auto p = Point { 1, 3 };
    EXPECT(p.translated(1) == Point { 2, 4 });
    EXPECT(p.translated(1, 0) == Point { 2, 3 });
    EXPECT(p.translated(0, 1) == Point { 1, 4 });
    EXPECT(p.translated(Point { 2, 2 }) == Point { 3, 5 });

    EXPECT(p.with_x(0) == Point { 0, 3 });
    EXPECT(p.with_y(0) == Point { 1, 0 });
}
