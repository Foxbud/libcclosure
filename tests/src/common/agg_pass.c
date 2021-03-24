/* Verify that closures properly handle an aggregate argument type.  */

#include "test_prelude.h"

typedef struct Doohickey {
    int64_t a;
    int64_t b;
    int64_t c;
} Doohickey;

static int64_t Callback(CClosureCtx ctx, Doohickey val) {
    int64_t *env = ctx.env;

    return val.a * *env + val.b * *env + val.c * *env;
}

TestCase {
    int64_t env = 2;
    int64_t (*clos)(Doohickey) = CClosureNew(Callback, &env, false);

    Doohickey input = (Doohickey){1, 2, 3};
    AssertIntEqual(clos(input), (int64_t)12);
    CClosureFree(clos);

    Pass();
}