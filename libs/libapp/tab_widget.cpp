#include <app/event.h>
#include <app/tab_widget.h>
#include <app/window.h>
#include <assert.h>
#include <graphics/renderer.h>

namespace App {

constexpr int character_height = 16;
constexpr int character_width = 8;

constexpr int tab_padding = 2;
constexpr int tab_border = 1;
constexpr int tab_bar_height = 2 * (tab_border + tab_padding) + character_height;
constexpr int tab_bar_left_margin = 4;

void TabWidget::on_mouse_event(MouseEvent& event) {
    if (event.left() == MOUSE_DOWN) {
        for (int i = 0; i < m_tabs.size(); i++) {
            if (m_tabs[i].rect.intersects({ event.x(), event.right() })) {
                set_active_tab(i);
                return;
            }
        }
    }

    return Widget::on_mouse_event(event);
}

void TabWidget::on_focused() {
    if (m_active_tab != -1) {
        window()->set_focused_widget(m_tabs[m_active_tab].widget.get());
    }
}

void TabWidget::on_resize() {
    Rect tab_content_rect = { rect().x(), rect().y() + tab_bar_height, rect().width(), rect().height() - tab_bar_height };
    if (tab_content_rect.height() < 0) {
        tab_content_rect = { 0, 0, 0, 0 };
    }
    for (auto& tab : m_tabs) {
        tab.widget->set_rect(tab_content_rect);
    }
    m_tab_content_rect = tab_content_rect;
}

void TabWidget::render() {
    Renderer renderer(*window()->pixels());
    for (auto& tab : m_tabs) {
        Rect absolute_rect = { rect().x() + tab.rect.x(), rect().y() + tab.rect.y(), tab.rect.width(), tab.rect.height() };
        renderer.draw_rect(absolute_rect, ColorValue::White);

        renderer.render_text(rect().x() + tab.rect.x() + tab_border + tab_padding, rect().y() + tab.rect.y() + tab_border + tab_padding,
                             tab.name, ColorValue::White);
    }

    Widget::render();
}

void TabWidget::set_active_tab(int index) {
    if (m_active_tab == index) {
        return;
    }

    if (m_active_tab != -1) {
        m_tabs[m_active_tab].widget->set_hidden(true);
    }
    m_active_tab = index;
    if (m_active_tab != -1) {
        m_tabs[m_active_tab].widget->set_hidden(false);
        window()->set_focused_widget(m_tabs[m_active_tab].widget.get());
    }
}

void TabWidget::remove_tab(int index) {
    assert(index >= 0 && index < m_tabs.size());
    if (m_tabs.size() == 1) {
        set_active_tab(-1);
    } else {
        set_active_tab((index + 1) % m_tabs.size());
    }

    int old_tab_width = m_tabs[index].rect.width();
    m_tabs.remove(index);

    for (int i = index; i < m_tabs.size(); i++) {
        auto& rect = m_tabs[i].rect;
        rect.set_x(rect.x() - old_tab_width);
    }
}

void TabWidget::did_remove_child(SharedPtr<Object> child) {
    int i = 0;
    for (auto& tab : m_tabs) {
        if (tab.widget.get() == child.get()) {
            remove_tab(i);
            break;
        }
        i++;
    }
}

Rect TabWidget::next_tab_rect(const String& name) const {
    auto text_width = static_cast<int>(name.size()) * character_width;
    auto width = 2 * (tab_border + tab_padding) + text_width;

    if (m_tabs.size() == 0) {
        return { tab_bar_left_margin, 0, width, tab_bar_height };
    }

    auto& reference_rect = m_tabs.last().rect;
    return { reference_rect.x() + reference_rect.width(), 0, width, tab_bar_height };
}

}
