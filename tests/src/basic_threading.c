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
#include <pthread.h>

#include "test_prelude.h"

static int32_t Callback(CClosureCtx ctx, int32_t val) {
    return *(int32_t *)ctx.env * val;
}

static void *ThreadCallClosure(void *ctx) {
    int32_t (*closure)(int32_t) = ctx;
    for (size_t idx = 0; idx < 1000000; idx++) {
        AssertIntEqual(closure(idx), (int32_t)(idx * 42));
    }

    return ctx;
}

TestCase {
    pthread_t thread0 = {0};
    pthread_t thread1 = {0};

    int32_t env = 42;
    void *closure = CClosureNew(Callback, &env, false);

    pthread_create(&thread0, NULL, ThreadCallClosure, closure);
    pthread_create(&thread1, NULL, ThreadCallClosure, closure);
    pthread_join(thread0, NULL);
    pthread_join(thread1, NULL);

    CClosureFree(closure);

    Pass();
}