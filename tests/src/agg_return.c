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

static Doohickey Callback0(CClosureCtx ctx, int64_t coef) {
    Doohickey *env = ctx.env;

    return (Doohickey){
        .a = env->a * coef,
        .b = env->b * coef,
        .c = env->c * coef,
    };
}

TestCase {
    Doohickey env0 = {1, 2, 3};
    Doohickey (*clos0)(int64_t) = CClosureNew(Callback0, &env0, true);

    Doohickey ret = clos0(2);
    AssertInt32Equal(ret.a, 2);
    AssertInt32Equal(ret.b, 4);
    AssertInt32Equal(ret.c, 6);
    CClosureFree(clos0);

    Pass();
}