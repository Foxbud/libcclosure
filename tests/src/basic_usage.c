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

typedef struct Line {
    double m;
    double b;
} Line;

static double LineGetY(CClosureCtx ctx, double x) {
    Line *line = ctx.env;

    return line->m * x + line->b;
}

TestCase {
    Line *env = NULL;
    const double EP = 0.00001;

    double (*clos0)(double) =
        CClosureNew(LineGetY, malloc(sizeof(Line)), false);
    env = CClosureGetEnv(clos0);
    env->m = 1.0;
    env->b = 0.0;
    AssertDoubleEqual(clos0(1.0), 1.0, EP);
    AssertDoubleEqual(clos0(-33.0), -33.0, EP);

    double (*clos1)(double) =
        CClosureNew(LineGetY, malloc(sizeof(Line)), false);
    env = CClosureGetEnv(clos1);
    env->m = -3.0;
    env->b = 10.0;
    AssertDoubleEqual(clos1(1.0), 7.0, EP);
    AssertDoubleEqual(clos1(-33.0), 109.0, EP);

    AssertDoubleEqual(clos0(0.0), 0.0, EP);
    free(CClosureFree(clos0));
    AssertDoubleEqual(clos1(0.0), 10.0, EP);

    free(CClosureFree(clos1));

    Pass();
}