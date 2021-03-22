/*
 * Copyright 2021 Garrett Fairburn <breadboardfox@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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