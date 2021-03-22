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

#define NUM_CLOSURES ((size_t)1000000)
static int32_t (*closures[NUM_CLOSURES])(void) = {0};

static int32_t Callback(CClosureCtx ctx) { return *(int32_t *)ctx.env; }

TestCase {
    for (size_t idx = 0; idx < NUM_CLOSURES; idx++) {
        int32_t *env = malloc(sizeof(int32_t));
        *env = idx * -2;
        AssertBoolEqual(CClosureCheck(closures[idx]), false);
        closures[idx] = CClosureNew(Callback, env, false);
        AssertBoolEqual(CClosureCheck(closures[idx]), true);
    }

    for (size_t idx = 0; idx < NUM_CLOSURES; idx++) {
        AssertBoolEqual(CClosureCheck(closures[idx]), true);
        AssertInt32Equal(closures[idx](), idx * -2);
    }

    for (size_t idx = 0; idx < NUM_CLOSURES; idx += 2) {
        free(CClosureFree(closures[idx]));
        AssertBoolEqual(CClosureCheck(closures[idx]), false);
    }

    for (size_t idx = 1; idx < NUM_CLOSURES; idx += 2) {
        AssertBoolEqual(CClosureCheck(closures[idx]), true);
        AssertInt32Equal(closures[idx](), idx * -2);
    }

    int32_t env = 42;
    int32_t (*clos)(void) = CClosureNew(Callback, &env, false);
    AssertBoolEqual(CClosureCheck(clos), true);
    AssertInt32Equal(clos(), 42);
    CClosureFree(clos);
    AssertBoolEqual(CClosureCheck(clos), false);

    for (size_t idx = 1; idx < NUM_CLOSURES; idx += 2) {
        free(CClosureFree(closures[idx]));
        AssertBoolEqual(CClosureCheck(closures[idx]), false);
    }

    Pass();
}