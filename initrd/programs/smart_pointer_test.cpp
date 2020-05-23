#include <liim/pointers.h>
#include <stdio.h>

using namespace LIIM;

class Test {
public:
    Test() {}
    ~Test() { fprintf(stderr, "Deconstructing\n"); }

    void test() { fprintf(stderr, "Test method\n"); }
};

int main() {
    auto test = UniquePtr<Test>(new Test());
    test->test();

    auto test2 = SharedPtr<Test>(new Test());
    auto test3 = test2;

    test3->test();
    test2->test();

    WeakPtr<Test> w1(test2);
    assert(!w1.expired());
    w1.lock()->test();

    auto test4 = w1.lock();

    test2 = nullptr;
    test3 = nullptr;

    test4->test();
    test4 = nullptr;

    assert(w1.expired());
    assert(!w1.lock());
}
