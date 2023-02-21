__extension__ typedef int raw_i128 __attribute__((mode(TI)));
__extension__ typedef unsigned raw_u128 __attribute__((mode(TI)));

extern "C" raw_u128 __udivti3(raw_u128, raw_u128) {
    return 0;
}
extern "C" raw_u128 __umodti3(raw_u128, raw_u128) {
    return 0;
}
extern "C" raw_i128 __divti3(raw_i128, raw_i128) {
    return 0;
}
