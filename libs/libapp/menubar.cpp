#include <app/box_layout.h>
#include <app/button.h>
#include <app/context_menu.h>
#include <app/menubar.h>
#include <app/window.h>
#include <graphics/renderer.h>

constexpr int menubar_height = 20;

namespace App {
class MenubarItem final : public Widget {
    APP_OBJECT(MenubarItem)

public:
    explicit MenubarItem(String name) : m_name(move(name)) {}
    virtual void initialize() override {
        m_menu = ContextMenu::create(shared_from_this(), window()->shared_from_this());
        m_button = Button::create(shared_from_this(), m_name);
        m_button->on_click = [this] {
            if (m_menu->menu_items().empty()) {
                return;
            }

            if (!m_shown) {
                m_menu->show({ positioned_rect().x(), positioned_rect().y() + positioned_rect().height() });
                m_shown = true;
            } else {
                m_shown = false;
            }
        };
    }
    virtual ~MenubarItem() override {}

    virtual void on_resize() override { m_button->set_positioned_rect(positioned_rect()); }

    ContextMenu& menu() { return *m_menu; }

private:
    SharedPtr<ContextMenu> m_menu;
    SharedPtr<Button> m_button;
    String m_name;
    bool m_shown { false };
};

Menubar::Menubar() {}

void Menubar::initialize() {
    set_preferred_size({ Size::Auto, menubar_height });

    auto& layout = set_layout<HorizontalBoxLayout>();
    layout.set_margins({ 0, 0, 0, 0 });
    layout.set_spacing(0);
}

ContextMenu& Menubar::create_menu(String name) {
    auto& item = layout()->add<MenubarItem>(name);
    return item.menu();
}

Menubar::~Menubar() {}
}
