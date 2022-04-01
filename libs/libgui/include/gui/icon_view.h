#pragma once

#include <graphics/bitmap.h>
#include <graphics/rect.h>
#include <gui/view.h>
#include <liim/vector.h>

namespace GUI {
class IconView : public View {
    APP_WIDGET(View, IconView)

public:
    IconView();
    virtual void did_attach() override;
    virtual ~IconView() override;

    virtual void render() override;

    void set_name_column(int col) { m_name_column = col; }

private:
    virtual App::ModelItem* item_at_position(const Point& point) override;
    virtual void install_model_listeners(App::Model& model) override;
    virtual void uninstall_model_listeners(App::Model& model) override;

    void rebuild_layout();
    void compute_layout();

    struct Item {
        SharedPtr<Bitmap> icon;
        String name;
        Rect rect;
        App::ModelItem* item;
    };

    Vector<Item> m_items;
    Point m_selection_start;
    Point m_selection_end;
    bool m_in_selection { false };
    int m_name_column { 0 };
    int m_icon_width { 32 };
    int m_icon_height { 32 };
    int m_icon_padding_x { 28 };
    int m_icon_padding_y { 8 };
};
}
