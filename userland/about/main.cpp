#include <app/app.h>
#include <app/box_layout.h>
#include <app/button.h>
#include <app/event.h>
#include <app/object.h>
#include <app/text_label.h>
#include <app/widget.h>
#include <app/window.h>
#include <assert.h>
#include <unistd.h>

class TestWidget : public App::Widget {
    APP_OBJECT(TestWidget)

private:
    virtual void on_key_event(App::KeyEvent& event) override {
        if (event.key_down()) {
            printf("pressed: '%c'\n", event.ascii());
        }
    }
};

int main() {
    App::App app;

    auto window = App::Window::create(nullptr, 300, 300, 250, 250, "About");

    auto& layout = window->set_layout<App::BoxLayout>(App::BoxLayout::Orientation::Vertical);
    layout.add<App::TextLabel>("Hello World!");
    layout.add<App::Widget>();
    layout.add<TestWidget>();
    auto& button = layout.add<App::Button>("Click Me!");
    button.on_click = [] {
        printf("clicked!\n");
    };

    window->draw();

    app.enter();
    return 0;
}
