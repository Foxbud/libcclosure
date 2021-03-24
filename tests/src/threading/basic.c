/* Verify that two threads can call the same closure at the same time without
 * issues. */

#include <pthread.h>

#include "test_prelude.h"

static int32_t Callback(CClosureCtx ctx, int32_t val) {
    return *(int32_t *)ctx.env * val;
}

static void *ThreadCallClosure(void *ctx) {
    int32_t (*closure)(int32_t) = ctx;
    for (size_t idx = 0; idx < 1000000; idx++) {
        AssertIntEqual(closure(idx), (int32_t)(idx * 42));
    }

    return ctx;
}

TestCase {
    pthread_t thread0 = {0};
    pthread_t thread1 = {0};

    int32_t env = 42;
    void *closure = CClosureNew(Callback, &env, false);

    pthread_create(&thread0, NULL, ThreadCallClosure, closure);
    pthread_create(&thread1, NULL, ThreadCallClosure, closure);
    pthread_join(thread0, NULL);
    pthread_join(thread1, NULL);

    CClosureFree(closure);

    Pass();
}