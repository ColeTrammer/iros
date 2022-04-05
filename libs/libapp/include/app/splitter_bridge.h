#include <app/splitter_bridge_interface.h>

namespace App {
class SplitterBridge {
public:
    virtual ~SplitterBridge() {}

    // os_2 reflect begin
    virtual int gutter_width() const = 0;
    // os_2 reflect end
};
}
