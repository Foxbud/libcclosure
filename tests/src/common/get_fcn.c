/* Verify that CClosureGetFcn works as expected. */

#include "test_prelude.h"

static void Callback(CClosureCtx ctx) {
    (void)ctx;

    return;
}

TestCase {
    void (*closure)(void) = CClosureNew(Callback, NULL, false);
    AssertIs(CClosureGetFcn(closure), &Callback);
    CClosureFree(closure);

    Pass();
}