/* Verify that execution is not impacted when a closure with a frame on the
 * stack is freed. */

#include "test_prelude.h"

static int64_t MAGIC = 73982007135488;

static int64_t Callback0(CClosureCtx ctx) {
    AssertBoolEqual(CClosureCheck(*(void **)ctx.env), true);
    CClosureFree(*(void **)ctx.env);
    AssertBoolEqual(CClosureCheck(*(void **)ctx.env), false);

    return MAGIC;
}

TestCase {
    int64_t (*closure)(void);
    closure = CClosureNew(Callback0, &closure, false);

    AssertBoolEqual(CClosureCheck(closure), true);
    AssertIntEqual(closure(), MAGIC);
    AssertBoolEqual(CClosureCheck(closure), false);

    Pass();
}