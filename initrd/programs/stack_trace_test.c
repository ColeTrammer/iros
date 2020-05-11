__attribute__((noinline)) void c() {
    *((char*) (0)) = '\0';
}

__attribute__((noinline)) void b() {
    c();
}

__attribute__((noinline)) void a() {
    b();
}

__attribute__((noinline)) int main() {
    a();

    return 0;
}
