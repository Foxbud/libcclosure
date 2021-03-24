/* Verify that closures properly handle an aggregate return type. */

#include "test_prelude.h"

typedef struct Doohickey {
    int64_t a;
    int64_t b;
    int64_t c;
} Doohickey;

static Doohickey Callback0(CClosureCtx ctx, int64_t coef) {
    Doohickey *env = ctx.env;

    return (Doohickey){
        .a = env->a * coef,
        .b = env->b * coef,
        .c = env->c * coef,
    };
}

TestCase {
    Doohickey env0 = {1, 2, 3};
    Doohickey (*clos0)(int64_t) = CClosureNew(Callback0, &env0, true);

    Doohickey ret = clos0(2);
    AssertIntEqual(ret.a, (int64_t)2);
    AssertIntEqual(ret.b, (int64_t)4);
    AssertIntEqual(ret.c, (int64_t)6);
    CClosureFree(clos0);

    Pass();
}