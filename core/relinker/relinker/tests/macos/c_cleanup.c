/* Guest C code built with -fexceptions: a C++ exception that passes through it runs its cleanup
   handlers through the C personality (__gcc_personality_v0). */
struct CleanupCounter { int* count; };

static void Leave(struct CleanupCounter* counter) { ++*counter->count; }

void CallWithCleanup(void (*function)(void), int* count) {
    [[gnu::cleanup(Leave)]] struct CleanupCounter counter = {count};
    function();
}
