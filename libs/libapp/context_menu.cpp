#include <app/box_layout.h>
#include <app/button.h>
#include <app/context_menu.h>
#include <app/window.h>
#include <graphics/point.h>
#include <liim/function.h>

namespace App {
class ContextMenuWindow final : public Window {
    APP_OBJECT(ContextMenuWindow)

public:
    ContextMenuWindow(Point p, ContextMenu& menu)
        : Window(p.x(), p.y(), 150, menu.menu_items().size() * 25, "Context Menu", false, WindowServer::WindowType::Frameless,
                 menu.parent_window()->wid())
        , m_context_menu(menu) {}

private:
    virtual void did_become_inactive() override { m_context_menu.hide(); }

    ContextMenu& m_context_menu;
};

ContextMenu::ContextMenu(SharedPtr<Window> parent_window) : m_parent_window(parent_window) {}

ContextMenu::~ContextMenu() {}

void ContextMenu::add_menu_item(String name, Function<void()> hook) {
    m_menu_items.add({ move(name), move(hook) });
}

Window& ContextMenu::ensure_window(Point p) {
    if (!m_window) {
        m_window = ContextMenuWindow::create(shared_from_this(), p, *this);
        auto& main_widget = m_window->set_main_widget<App::Widget>();
        auto& layout = main_widget.set_layout<App::VerticalBoxLayout>();
        layout.set_margins({ 0, 0, 0, 0 });
        layout.set_spacing(0);

        for (auto& item : m_menu_items) {
            auto& button = layout.add<Button>(item.name);
            button.on<App::ClickEvent>(*this, [this, &item](auto&) {
                hide();
                item.on_click();
            });
        }
    }
    return *m_window;
}

void ContextMenu::show(Point p) {
    if (!visible()) {
        auto& window = ensure_window(p);
        window.show(p.x(), p.y());
        m_parent_window->set_current_context_menu(this);
    }
}

void ContextMenu::hide() {
    if (visible()) {
        m_window->hide();
        m_parent_window->clear_current_context_menu();
    }
}

bool ContextMenu::visible() const {
    return m_window && m_window->visible();
}
}
