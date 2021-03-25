/* Verify that CClosureGetFcn works as expected. */

#include "test_prelude.h"

typedef struct Doohickey {
    int64_t a;
    int64_t b;
    int64_t c;
} Doohickey;

static void CallbackNorm(CClosureCtx ctx) {
    (void)ctx;

    return;
}

static Doohickey CallbackAgg(CClosureCtx ctx) {
    (void)ctx;

    return (Doohickey){0};
}

TestCase {
    void (*closNorm)(void) = CClosureNew(CallbackNorm, NULL, false);
    Doohickey (*closAgg)(void) = CClosureNew(CallbackAgg, NULL, true);

    AssertIs(CClosureGetFcn(closNorm), &CallbackNorm);
    AssertIs(CClosureGetFcn(closAgg), &CallbackAgg);

    CClosureFree(closNorm);
    CClosureFree(closAgg);

    Pass();
}