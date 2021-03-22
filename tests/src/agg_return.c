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
    AssertInt32Equal(ret.a, 2);
    AssertInt32Equal(ret.b, 4);
    AssertInt32Equal(ret.c, 6);
    CClosureFree(clos0);

    Pass();
}