/* Verify that 1,000,000 closures can be allocated, used, and freed in different
 * orders and still work as expected. */

#include "test_prelude.h"

#define NUM_CLOSURES ((size_t)1000000)
static int32_t (*closures[NUM_CLOSURES])(void) = {0};

static int32_t Callback(CClosureCtx ctx) {
    return *(int32_t*)ctx.env;
}

TestCase {
    for (size_t idx = 0; idx < NUM_CLOSURES; idx++) {
        int32_t* env = malloc(sizeof(int32_t));
        *env = idx * -2;
        AssertBoolEqual(CClosureCheck(closures[idx]), false);
        closures[idx] = CClosureNew(Callback, env, false);
        AssertBoolEqual(CClosureCheck(closures[idx]), true);
    }

    for (size_t idx = 0; idx < NUM_CLOSURES; idx++) {
        AssertBoolEqual(CClosureCheck(closures[idx]), true);
        AssertIntEqual(closures[idx](), (int32_t)(idx * -2));
    }

    for (size_t idx = 0; idx < NUM_CLOSURES; idx += 2) {
        free(CClosureFree(closures[idx]));
        AssertBoolEqual(CClosureCheck(closures[idx]), false);
    }

    for (size_t idx = 1; idx < NUM_CLOSURES; idx += 2) {
        AssertBoolEqual(CClosureCheck(closures[idx]), true);
        AssertIntEqual(closures[idx](), (int32_t)(idx * -2));
    }

    int32_t env = 42;
    int32_t (*clos)(void) = CClosureNew(Callback, &env, false);
    AssertBoolEqual(CClosureCheck(clos), true);
    AssertIntEqual(clos(), (int32_t)42);
    CClosureFree(clos);
    AssertBoolEqual(CClosureCheck(clos), false);

    for (size_t idx = 1; idx < NUM_CLOSURES; idx += 2) {
        free(CClosureFree(closures[idx]));
        AssertBoolEqual(CClosureCheck(closures[idx]), false);
    }

    Pass();
}