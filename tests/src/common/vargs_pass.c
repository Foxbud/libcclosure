/* Verify that closures properly handle variadic arguments. */

#include "test_prelude.h"

static char *Callback(CClosureCtx ctx, const char *fmt, size_t size, ...) {
    const char *env = ctx.env;

    char *tmp = malloc(size);
    va_list args;
    va_start(args, size);
    vsnprintf(tmp, size, fmt, args);
    va_end(args);

    char *result = malloc(size);
    snprintf(result, size, "%s%s", env, tmp);
    free(tmp);

    return result;
}

TestCase {
    char *result = NULL;

    char *(*clos0)(const char *, size_t, ...) =
        CClosureNew(Callback, "Closure 0: ", false);
    AssertStrEqual((result = clos0("%i, %s, %.2f", 128, 42, "test", 3.14)),
                   "Closure 0: 42, test, 3.14");
    free(result);

    Pass();
}