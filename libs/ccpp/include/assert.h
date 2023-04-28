#undef assert

#ifdef NDEBUG
#define assert(condition) ((void) 0)
#else
// FIXME: Actually implement assert()
#define assert(condition) ((void) 0)
#endif
