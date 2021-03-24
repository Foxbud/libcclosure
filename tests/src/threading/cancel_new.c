/* Verify that a thread running CClosureNew can be canceled without issues. */

#include <pthread.h>

#include "test_prelude.h"

typedef struct ThreadCreateState {
    size_t idx;
    size_t *num;
    size_t *env;
} ThreadCreateState;

#define NUM_CLOSURES ((size_t)10000)
static size_t (*closures[NUM_CLOSURES])(size_t) = {0};

static size_t Callback(CClosureCtx ctx, size_t val) {
    return *(size_t *)ctx.env * val;
}

static void ThreadCreateClosureRollback(void *ctx) {
    ThreadCreateState *state = ctx;

    *state->num = state->idx;
    if (state->env != NULL) {
        free(state->env);
        if (CClosureCheck(closures[state->idx]))
            CClosureFree(closures[state->idx]);
    }

    return;
}

static void *ThreadCreateClosure(void *ctx) {
    int oldType;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldType);

    size_t *num = ctx;
    for (size_t idx = 0; idx < NUM_CLOSURES; idx++) {
        closures[idx] = NULL;
        ThreadCreateState state = {
            .idx = idx,
            .num = num,
            .env = NULL,
        };
        pthread_cleanup_push(ThreadCreateClosureRollback, &state);
        (*num)++;
        state.env = malloc(sizeof(size_t));
        *state.env = idx;
        closures[idx] = CClosureNew(Callback, state.env, false);
        pthread_cleanup_pop(false);
    }

    return num;
}

TestCase {
    pthread_t thread = {0};

    size_t num = 0;
    pthread_create(&thread, NULL, ThreadCreateClosure, &num);
    AssertIntEqual(pthread_cancel(thread), 0);
    int retVal;
    AssertIntEqual(pthread_join(thread, (void *)&retVal), 0);
    AssertIntGreater(num, 0);
    AssertIntLess(num, NUM_CLOSURES);

    Pass();
}