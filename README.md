# libcclosure

**libcclosure** is a library which adds thread-safe [closures](https://en.wikipedia.org/wiki/Closure_(computer_programming)) as first-class functions to the C language.

## Quick Start

Suppose an external API provides some function that accepts a callback function:

```c
/* list.h */
#include <stdbool.h>
#include <stddef.h>

typedef void List;

List *ListCreate(size_t num, ...);
void ListForEach(List *list, bool (*callback)(int *element));
```

Functions like this typically accept a "data" argument to pass to callback in addition to `element`, but imagine that isn't the case here. That functionality can be recreated using a closure:

```c
/* main.c */
#include <stdio.h>
#include <stdlib.h>

#include "cclosure.h"
#include "lists.h"

/* Type of closure environment. */
struct SumGreaterThanEnv {
    int sum;
    int threshold;
};

/* Function that accepts closure context as first argument. */
static bool SumGreaterThanCallback(CClosureCtx ctx, int *element) {
    /* Closure context contains the bound environment. */
    SumGreaterThanEnv *env = ctx.env;

    if (*element > env->threshold)
        env->sum += *element;

    return true;
}

int main(void) {
    List *list = ListCreate(5, 3, -10, 77, 42, 15);

    /* Instantiate an environment for the closure. */
    SumGreaterThanEnv *env = malloc(sizeof(SumGreaterThanEnv));
    *env = (SumGreaterThanEnv){
        .sum = 0,
        .threshold = 10
    };

    /* Create a closure by binding environment to callback. */
    bool (*callback)(int *) = CClosureNew(SumGreaterThanCallback, env, false);

    /* Callback is now a first-class function that will be passed environment
       implicitly to its first parameter. */
    ListForEach(list, callback);

    /* Would print "134". */
    printf("%i", env->sum);

    /* "CClosureFree" returns the closure's environment. */
    free(CClosureFree(callback));

    return 0;
}
```