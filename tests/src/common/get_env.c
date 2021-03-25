/* Verify that CClosureGetEnv works as expected and that CClosureFree returns
 * previously-bound environment.. */

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
    int32_t envNorm = 42;
    int32_t envAgg = -42;

    void (*closNorm)(void) = CClosureNew(CallbackNorm, &envNorm, false);
    Doohickey (*closAgg)(void) = CClosureNew(CallbackAgg, &envAgg, true);

    AssertIs(CClosureGetEnv(closNorm), &envNorm);
    AssertIs(CClosureGetEnv(closAgg), &envAgg);

    AssertIs(CClosureFree(closNorm), &envNorm);
    AssertIs(CClosureFree(closAgg), &envAgg);

    Pass();
}