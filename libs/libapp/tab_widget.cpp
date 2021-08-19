#include <app/tab_widget.h>
#include <app/window.h>
#include <assert.h>
#include <eventloop/event.h>
#include <graphics/renderer.h>

namespace App {

constexpr int character_height = 16;
constexpr int character_width = 8;

constexpr int tab_padding = 2;
constexpr int tab_border = 1;
constexpr int tab_bar_height = 2 * (tab_border + tab_padding) + character_height;
constexpr int tab_bar_left_margin = 4;

void TabWidget::initialize() {
    on<MouseDownEvent>([this](const MouseDownEvent& event) {
        if (event.left_button()) {
            for (int i = 0; i < m_tabs.size(); i++) {
                if (m_tabs[i].rect.intersects({ event.x(), event.y() })) {
                    set_active_tab(i);
                    return true;
                }
            }
        }
        return false;
    });

    // FIXME: use some sort of focus proxy mechanism instead.
    on<FocusedEvent>([this](const FocusedEvent&) {
        if (m_active_tab != -1) {
            window()->set_focused_widget(m_tabs[m_active_tab].widget.get());
        }
    });

    on<ResizeEvent>([this](const ResizeEvent&) {
        Rect tab_content_rect = { positioned_rect().x(), positioned_rect().y() + tab_bar_height, positioned_rect().width(),
                                  positioned_rect().height() - tab_bar_height };
        if (tab_content_rect.height() < 0) {
            tab_content_rect = { 0, 0, 0, 0 };
        }
        for (auto& tab : m_tabs) {
            tab.widget->set_positioned_rect(tab_content_rect);
        }
        m_tab_content_rect = tab_content_rect;
    });

    Widget::initialize();
}

void TabWidget::render() {
    auto renderer = get_renderer();

    renderer.clear_rect({ 0, 0, sized_rect().width(), tab_bar_height }, background_color());
    for (auto& tab : m_tabs) {
        Rect absolute_rect = { tab.rect.x(), tab.rect.y(), tab.rect.width(), tab.rect.height() };
        renderer.draw_rect(absolute_rect, outline_color());
        renderer.render_text(tab.name, absolute_rect.adjusted(-tab_padding), text_color());
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
