#ifndef TEST_PRELUDE_H
#define TEST_PRELUDE_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cclosure.h"

/* ----- INTERNAL MACROS ----- */

#define TestCase int32_t main(void)

#define Pass() exit(PASSED)

#define Fail(fmt, ...)                                                         \
    do {                                                                       \
        fprintf(stderr, fmt, ##__VA_ARGS__);                                   \
        exit(FAILED);                                                          \
    } while (0)

#define AssertBoolEqual(result, expected)                                      \
    do {                                                                       \
        bool Assert_result = (result);                                         \
        bool Assert_expected = (expected);                                     \
        if (Assert_result != Assert_expected) {                                \
            Fail("AssertBoolEqual @%i failed!\n%s == %s\n(%s == %s)\n",        \
                 __LINE__, (Assert_result) ? "true" : "false",                 \
                 (Assert_expected) ? "true" : "false", #result, #expected);    \
        }                                                                      \
    } while (0)

#define AssertInt32Equal(result, expected)                                     \
    do {                                                                       \
        int32_t Assert_result = (result);                                      \
        int32_t Assert_expected = (expected);                                  \
        if (Assert_result != Assert_expected) {                                \
            Fail("AssertInt32Equal @%i failed!\n%i == %i\n(%s == %s)\n",       \
                 __LINE__, Assert_result, Assert_expected, #result,            \
                 #expected);                                                   \
        }                                                                      \
    } while (0)

#define AssertDoubleEqual(result, expected, epsilon)                           \
    do {                                                                       \
        double Assert_result = (result);                                       \
        double Assert_expected = (expected);                                   \
        double Assert_epsilon = (epsilon);                                     \
        if (Assert_result > Assert_expected + Assert_epsilon ||                \
            Assert_result < Assert_expected - Assert_epsilon) {                \
            Fail("AssertDoubleEqual @%i failed!\n%e == %e +/- %e\n(%s == %s "  \
                 "+/- %s)\n",                                                  \
                 __LINE__, Assert_result, Assert_expected, Assert_epsilon,     \
                 #result, #expected, #epsilon);                                \
        }                                                                      \
    } while (0)

#define AssertStrEqual(result, expected)                                       \
    do {                                                                       \
        const char *Assert_result = (result);                                  \
        const char *Assert_expected = (expected);                              \
        if (strcmp(Assert_result, Assert_expected) != 0) {                     \
            Fail("AssertStrEqual @%i failed!\n\"%s\" == \"%s\"\n(%s == %s)\n", \
                 __LINE__, Assert_result, Assert_expected, #result,            \
                 #expected);                                                   \
        }                                                                      \
    } while (0)

/* ----- INTERNAL TYPES ----- */

typedef enum TestResult {
    PASSED = 0,
    FAILED = 1,
} TestResult;

#endif /* TEST_PRELUDE_H */