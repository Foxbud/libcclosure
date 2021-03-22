#include "test_prelude.h"

TestCase {
    AssertBoolEqual(CCLOSURE_THREAD_TYPE, THREAD_TYPE);

    Pass();
}