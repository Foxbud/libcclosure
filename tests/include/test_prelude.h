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

#ifdef __LP64__
#define INT64_FMT "%li"
#define UINT64_FMT "%lu"
#else
#define INT64_FMT "%lli"
#define UINT64_FMT "%llu"
#endif

#define TestCase int32_t main(void)

#define Pass() exit(PASSED)

#define Fail(fmt, ...)                                                         \
    do {                                                                       \
        fprintf(stderr, (fmt), ##__VA_ARGS__);                                 \
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

#define AssertIntLess(result, max)                                             \
    do {                                                                       \
        typeof(max) Assert_result = (result);                                  \
        typeof(max) Assert_max = (max);                                        \
        if (Assert_result >= Assert_max) {                                     \
            Fail(                                                              \
                _Generic((max), int64_t                                        \
                         : "AssertIntLess @%i failed!\n" INT64_FMT             \
                           " < " INT64_FMT "\n(%s < %s)\n",                    \
                           uint64_t                                            \
                         : "AssertIntLess @%i failed!\n" UINT64_FMT            \
                           " < " UINT64_FMT "\n(%s < %s)\n",                   \
                           int32_t                                             \
                         : "AssertIntLess @%i failed!\n%i < %i\n(%s < %s)\n",  \
                           uint32_t                                            \
                         : "AssertIntLess @%i failed!\n%u < %u\n(%s < %s)\n"), \
                __LINE__, Assert_result, Assert_max, #result, #max);           \
        }                                                                      \
    } while (0)

#define AssertIntGreater(result, min)                                          \
    do {                                                                       \
        typeof(min) Assert_result = (result);                                  \
        typeof(min) Assert_min = (min);                                        \
        if (Assert_result <= Assert_min) {                                     \
            Fail(_Generic(                                                     \
                     (min), int64_t                                            \
                     : "AssertIntGreater @%i failed!\n" INT64_FMT              \
                       " > " INT64_FMT "\n(%s > %s)\n",                        \
                       uint64_t                                                \
                     : "AssertIntGreater @%i failed!\n" UINT64_FMT             \
                       " > " UINT64_FMT "\n(%s > %s)\n",                       \
                       int32_t                                                 \
                     : "AssertIntGreater @%i failed!\n%i > %i\n(%s > %s)\n",   \
                       uint32_t                                                \
                     : "AssertIntGreater @%i failed!\n%u > %u\n(%s > %s)\n"),  \
                 __LINE__, Assert_result, Assert_min, #result, #min);          \
        }                                                                      \
    } while (0)

#define AssertIntEqual(result, expected)                                       \
    do {                                                                       \
        typeof(expected) Assert_result = (result);                             \
        typeof(expected) Assert_expected = (expected);                         \
        if (Assert_result != Assert_expected) {                                \
            Fail(_Generic(                                                     \
                     (expected), int64_t                                       \
                     : "AssertIntEqual @%i failed!\n" INT64_FMT                \
                       " == " INT64_FMT "\n(%s == %s)\n",                      \
                       uint64_t                                                \
                     : "AssertIntEqual @%i failed!\n" UINT64_FMT               \
                       " == " UINT64_FMT "\n(%s == %s)\n",                     \
                       int32_t                                                 \
                     : "AssertIntEqual @%i failed!\n%i == %i\n(%s == %s)\n",   \
                       uint32_t                                                \
                     : "AssertIntEqual @%i failed!\n%u == %u\n(%s == %s)\n"),  \
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

#define AssertIs(result, expected)                                             \
    do {                                                                       \
        void *Assert_result = (result);                                        \
        void *Assert_expected = (expected);                                    \
        if (Assert_result != Assert_expected) {                                \
            Fail("AssertIs @%i failed!\n0x%p == 0x%p\n(%s == %s)\n", __LINE__, \
                 Assert_result, Assert_expected, #result, #expected);          \
        }                                                                      \
    } while (0)

/* ----- INTERNAL TYPES ----- */

typedef enum TestResult {
    PASSED = 0,
    FAILED = 1,
} TestResult;

#endif /* TEST_PRELUDE_H */