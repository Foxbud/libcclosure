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

typedef struct Range {
    size_t start;
    size_t end;
    size_t stride;
} Range;

#define NUM_CLOSURES ((size_t)1000000)
static int32_t (*closures[NUM_CLOSURES])(void) = {0};

static int32_t Callback(CClosureCtx ctx) { return *(int32_t *)ctx.env; }

static void *ThreadCreateClosures(void *ctx) {
    Range range = *(Range *)ctx;
    for (size_t idx = range.start; idx < range.end; idx += range.stride) {
        int32_t *env = malloc(sizeof(int32_t));
        *env = idx * -2;
        AssertBoolEqual(CClosureCheck(closures[idx]), false);
        closures[idx] = CClosureNew(Callback, env, false);
        AssertBoolEqual(CClosureCheck(closures[idx]), true);
    }

    return ctx;
}

static void *ThreadCallClosures(void *ctx) {
    Range range = *(Range *)ctx;
    for (size_t idx = range.start; idx < range.end; idx += range.stride) {
        AssertBoolEqual(CClosureCheck(closures[idx]), true);
        AssertIntEqual(closures[idx](), (int32_t)(idx * -2));
    }

    return ctx;
}

static void *ThreadDestroyClosures(void *ctx) {
    Range range = *(Range *)ctx;
    for (size_t idx = range.start; idx < range.end; idx += range.stride) {
        free(CClosureFree(closures[idx]));
        AssertBoolEqual(CClosureCheck(closures[idx]), false);
    }

    return ctx;
}

static void *ThreadConditionalDestroyClosures(void *ctx) {
    Range range = *(Range *)ctx;
    for (size_t idx = range.start; idx < range.end; idx += range.stride) {
        if (!CClosureCheck(closures[idx]))
            continue;
        free(CClosureFree(closures[idx]));
        AssertBoolEqual(CClosureCheck(closures[idx]), false);
    }

    return ctx;
}

TestCase {
    Range *range = NULL;
    pthread_t auxThread = {0};
    pthread_t subThreads[5] = {0};

    range = malloc(sizeof(Range));
    *range = (Range){.start = 500000, .end = 1000000, .stride = 1};
    pthread_create(&auxThread, NULL, ThreadCreateClosures, range);
    for (size_t idx = 0; idx < 5; idx++) {
        range = malloc(sizeof(Range));
        *range = (Range){
            .start = idx * 100000, .end = (idx + 1) * 100000, .stride = 1};
        pthread_create(subThreads + idx, NULL, ThreadCreateClosures, range);
    }
    for (size_t idx = 0; idx < 5; idx++) {
        pthread_join(subThreads[idx], (void **)&range);
        pthread_create(subThreads + idx, NULL, ThreadCallClosures, range);
    }
    for (size_t idx = 0; idx < 5; idx++) {
        pthread_join(subThreads[idx], (void **)&range);
        free(range);
    }
    pthread_join(auxThread, (void **)&range);
    free(range);

    range = malloc(sizeof(Range));
    *range = (Range){.start = 5, .end = 1000000, .stride = 6};
    pthread_create(&auxThread, NULL, ThreadDestroyClosures, range);
    for (size_t idx = 0; idx < 5; idx++) {
        range = malloc(sizeof(Range));
        *range = (Range){.start = idx, .end = 1000000, .stride = 6};
        pthread_create(subThreads + idx, NULL, ThreadCallClosures, range);
    }
    pthread_join(auxThread, (void **)&range);
    free(range);

    for (size_t idx = 0; idx < 5; idx++) {
        pthread_join(subThreads[idx], (void **)&range);
        *range = (Range){
            .start = idx * 200000, .end = (idx + 1) * 200000, .stride = 1};
        pthread_create(subThreads + idx, NULL, ThreadConditionalDestroyClosures,
                       range);
    }
    for (size_t idx = 0; idx < 5; idx++) {
        pthread_join(subThreads[idx], (void **)&range);
        free(range);
    }

    Pass();
}