#pragma once

#include <app/direction.h>
#include <app/layout_engine.h>
#include <app/splitter_bridge.h>
#include <app/splitter_interface.h>
#include <app/widget.h>
#include <graphics/point.h>
#include <graphics/rect.h>
#include <liim/maybe.h>

namespace App::Detail {
struct HoldStart {
    Point origin;
    int item_index { 0 };
};

class SplitterLayoutEngine : public LayoutEngine {
public:
    explicit SplitterLayoutEngine(Widget& parent);
    virtual ~SplitterLayoutEngine() override;

    virtual void layout() override;
    virtual void do_add(Widget&) override { assert(false); }
    virtual void do_remove(Widget& widget) override;

    int gutter_width() const { return m_gutter_width; }
    void set_gutter_width(int gutter_width) { m_gutter_width = gutter_width; }

    Direction direction() const { return m_direction; }
    void set_direction(Direction direction) { m_direction = direction; }

    Maybe<HoldStart> compute_hold_start(const Point& origin) const;
    void adjust_size_and_position(const HoldStart& start, const Point& drag_point);

    template<typename WidgetType, typename... Args>
    SharedPtr<WidgetType> add(Maybe<int> fixed_width, Args&&... args) {
        auto result = WidgetType::create_owned(&parent(), forward<Args>(args)...);
        add_impl(fixed_width, result->base());
        schedule_layout();
        return result;
    }

private:
    struct Item {
        double expected_fraction;
        Rect relative_rect;
        SharedPtr<Widget> widget;
    };

    void add_impl(Maybe<int> fixed_width, Widget& widget);

    void insert_widget(Widget& widget, int index);
    void remove_widget_at_index(int index);
    Maybe<int> find_index_of_item(Widget& widget) const;

    int item_count() const { return m_items.size(); }
    int flexible_item_count() const;
    bool is_flexible_item(const Item& item) const;
    int rect_size_in_layout_direction(const Rect& rect) const;
    int rect_size_against_layout_direction(const Rect& rect) const;
    int available_space_in_layout_direction() const;
    int available_space_against_layout_direction() const;
    int flexible_space() const;
    void compute_layout();

    Direction m_direction { Direction::Horizontal };
    Vector<Item> m_items;
    int m_gutter_width { 0 };
};
}

namespace App {
class Splitter : public Widget {
    APP_OBJECT(Splitter)

    APP_SPLITTER_BRIDGE_INTERFACE_FORWARD(bridge())

public:
    Splitter(SharedPtr<WidgetBridge> widget_bridge, SharedPtr<SplitterBridge> splitter_bridge);
    virtual void initialize() override;
    virtual ~Splitter() override;

    // iros reflect begin
    App::Direction direction() const { return m_direction; }
    void set_direction(App::Direction direction);

    template<typename T, typename... Args>
    T& add_widget(Args&&... args) {
        return *add_widget_owned<T>(forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    SharedPtr<T> add_widget_owned(Args&&... args) {
        return layout().add<T>({}, forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    T& add_widget_fixed(int fixed_width, Args&&... args) {
        return *add_widget_fixed_owned<T>(fixed_width, forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    SharedPtr<T> add_widget_fixed_owned(int fixed_width, Args&&... args) {
        return layout().add<T>({ fixed_width }, forward<Args>(args)...);
    }
    // iros reflect end

    SplitterBridge& bridge() { return *m_bridge; }
    const SplitterBridge& bridge() const { return *m_bridge; }

private:
    Detail::SplitterLayoutEngine& layout() {
        return const_cast<Detail::SplitterLayoutEngine&>(const_cast<const Splitter&>(*this).layout());
    };
    const Detail::SplitterLayoutEngine& layout() const;

    SharedPtr<SplitterBridge> m_bridge;
    Direction m_direction { Direction::Horizontal };
    Maybe<Detail::HoldStart> m_hold_start;
};
}
