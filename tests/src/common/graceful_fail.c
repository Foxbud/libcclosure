/* Verify that invoking an uninitialized macro causes an illegal instruction
 * fault. */

#include <signal.h>

#include "test_prelude.h"

void Callback0(void) {
    return;
}

void SigIllHandler(int signum) {
    AssertIntEqual(signum, SIGILL);

    Pass();
}

TestCase {
    void (*closure)(void) = CClosureNew(Callback0, NULL, false);
    AssertBoolEqual(CClosureCheck(closure), true);
    CClosureFree(closure);
    AssertBoolEqual(CClosureCheck(closure), false);

    signal(SIGILL, SigIllHandler);
    closure();

    Fail("Execution did not abort!");
}