#include <app/app.h>
#include <app/box_layout.h>
#include <app/object.h>
#include <app/text_label.h>
#include <app/widget.h>
#include <app/window.h>
#include <assert.h>
#include <unistd.h>

int main() {
    App::App app;

    auto window = App::Window::create(nullptr, 300, 300, 250, 250, "About");

    auto& layout = window->set_layout<App::BoxLayout>(App::BoxLayout::Orientation::Vertical);
    layout.add<App::TextLabel>("Hello World!");
    layout.add<App::Widget>();
    layout.add<App::Widget>();
    layout.add<App::Widget>();

    window->draw();

    app.enter();
    return 0;
}
