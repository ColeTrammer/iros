#pragma once

namespace App {

class Selectable;

class EventLoop {
public:
    static void register_selectable(Selectable& selectable);
    static void unregister_selectable(Selectable& selectable);
};

};
