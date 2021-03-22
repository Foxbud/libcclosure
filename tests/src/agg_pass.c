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

typedef struct Doohickey {
    int64_t a;
    int64_t b;
    int64_t c;
} Doohickey;

static int64_t Callback(CClosureCtx ctx, Doohickey val) {
    int64_t *env = ctx.env;

    return val.a * *env + val.b * *env + val.c * *env;
}

TestCase {
    int64_t env = 2;
    int64_t (*clos)(Doohickey) = CClosureNew(Callback, &env, false);

    AssertInt32Equal(clos((Doohickey){1, 2, 3}), 12);
    CClosureFree(clos);

    Pass();
}