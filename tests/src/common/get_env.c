/* Verify that CClosureGetEnv works as expected and that CClosureFree returns
 * previously-bound environment.. */

#include "test_prelude.h"

TestCase {
    int32_t env = 42;
    void (*clos)(void) = CClosureNew(NULL, &env, false);
    AssertIs(CClosureGetEnv(clos), &env);
    AssertIs(CClosureFree(clos), &env);

    Pass();
}