#include "test_prelude.h"

TestCase {
    void *clos0 = NULL;

    AssertBoolEqual(CClosureCheck(clos0), false);
    clos0 = CClosureNew(NULL, NULL, false);
    AssertBoolEqual(CClosureCheck(clos0), true);
    CClosureFree(clos0);
    AssertBoolEqual(CClosureCheck(clos0), false);

    Pass();
}