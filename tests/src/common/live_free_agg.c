/* Verify that execution is not impacted when a closure with a frame on the
 * stack is freed. */

#include "test_prelude.h"

typedef struct Doohickey {
    int64_t a;
    int64_t b;
    int64_t c;
} Doohickey;

static int64_t MAGIC = 73982007135488;

static Doohickey Callback0(CClosureCtx ctx) {
    AssertBoolEqual(CClosureCheck(*(void **)ctx.env), true);
    CClosureFree(*(void **)ctx.env);
    AssertBoolEqual(CClosureCheck(*(void **)ctx.env), false);

    return (Doohickey){.a = MAGIC, .b = MAGIC - 1, .c = ~MAGIC};
}

TestCase {
    Doohickey (*closure)(void);
    closure = CClosureNew(Callback0, &closure, false);

    AssertBoolEqual(CClosureCheck(closure), true);
    Doohickey val = closure();
    AssertBoolEqual(CClosureCheck(closure), false);
    AssertIntEqual(val.a, MAGIC);
    AssertIntEqual(val.b, MAGIC - 1);
    AssertIntEqual(val.c, ~MAGIC);

    Pass();
}