#pragma once

#include <graphics/rect.h>
#include <liim/vector.h>

class RectSet {
public:
    RectSet() {}

    void add(const Rect& rect);
    void subtract(const Rect& rect);

    Vector<Rect>::Iterator begin() { return m_rects.begin(); }
    Vector<Rect>::Iterator end() { return m_rects.end(); }

    Vector<Rect>::ConstIterator begin() const { return m_rects.begin(); }
    Vector<Rect>::ConstIterator end() const { return m_rects.end(); }

    void clear() { m_rects.clear(); }

    bool intersects(const Point& point) const;
    bool intersects(const Rect& rect) const;

    void swap(RectSet& other) { LIIM::swap(this->m_rects, other.m_rects); }

private:
    Vector<Rect> m_rects;
};
