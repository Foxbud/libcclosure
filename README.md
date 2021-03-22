# libcclosure

**libcclosure** is a library which adds thread-safe [closures](https://en.wikipedia.org/wiki/Closure_(computer_programming)) as first-class functions to the C language.

This library is heavily inspired by and intended as a more permissively-licensed alternative to libffcall's [callback](https://www.gnu.org/software/libffcall/callback.html) module. If your project's license permits the use of [GPL 3.0-licensed](https://www.gnu.org/licenses/gpl-3.0.html) software, you should probably use libffcall instead; it has had more rigorous bug testing and supports a wider range of systems and architectures.

## Compatibility

### Supported Operating Systems
- Linux

### Supported ISAs
- x86 ([cdecl](https://en.wikipedia.org/wiki/X86_calling_conventions#cdecl))
- x86_64 ([sys v](https://en.wikipedia.org/wiki/X86_calling_conventions#System_V_AMD64_ABI))

### Supported Multi-Threading Libraries
- [POSIX Threads](https://en.wikipedia.org/wiki/POSIX_Threads)

## Build and Installation

This library uses [CMake](https://cmake.org/) to generate its build system.

### x86_64

```
$ cmake -S . -B build/release_x86_64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DBUILD_ARCH=x86_64
$ cmake --build build/release_x86_64
# cmake --build build/release_x86_64 --target install
```

### x86

```
$ cmake -S . -B build/release_x86 \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DBUILD_ARCH=x86
$ cmake --build build/release_x86
# cmake --build build/release_x86 --target install
```

## Quick Start

These closures really are first-class C functions in the sense that they can accept arbitrary arguments (including variadic) and have an arbitrary return type. To create one, first define a callback function that accepts a special closure "context" as its first argument followed by the other desired arguments:

```c
int Callback(CClosureCtx ctx, double filter, size_t numVArgs, ...) {
    /* "ctx.env" is a pointer to the closure's environment. */

    /* ... */
}
```

To create a closure, you must bind an environment to the callback function (pass `true` as the third argument to `CClosureNew` if the callback returns an [aggregate type](https://gcc.gnu.org/onlinedocs/gcc-3.4.2/gccint/Aggregate-Return.html) rather than a scalar):

```c
int (*closure)(double, size_t, ...) = CClosureNew(Callback, &someEnv, false);
```

`CClosureNew` is completely thread-safe assuming that libcclosure was compiled with multi-threading support.

`closure` can now be called like any other C function, and its bound environment will be implicitly passed to it before the arguments it was called with:

```c
int val0 = closure(15.0, 2, "some", "string");
int val1 = closure(8.0, 0);
```

The bound closure is thread-safe in the sense that multiple threads may safely call it in parallel and read from its environment. If the closure's callback modifies its environment, however, you must ensure that it does so in a thread-safe manner (like by using a mutex).

Use `CClosureCheck` to determine whether or not a given reference is to a bound closure:

```c
bool isClosure = CClosureCheck(closure);
```

Retrieve the environment bound to a closure using `CClosureGetEnv`:

```c
void *env = CClosureGetEnv(closure);
```

and retrieve the callback function bound to it using `CClosureGetFcn`:

```c
void *fcn = CClosureGetFcn(closure);
```

Use `CClosureFree` to de-allocate a bound closure:

```c
void *env = CClosureFree(closure);
```

Note that `CClosureFree` returns the previously-bound environment.

`CClosureFree` is thread-safe in the sense that multiple threads may safely call it (along with `CClosureNew`) in parallel. However, `CClosureFree` does **not** block if other threads (or even the same thread) are in the middle of calls to the closure. Make certain that the closure is no longer in use before de-allocating it.

Test whether or not libcclosure was compiled with multi-threading support using the `CCLOSURE_THREAD_TYPE` global:

```c
switch (CCLOSURE_THREAD_TYPE) {
    /* Compiled with multi-threading support using POSIX Threads. */
    case CCLOSURE_THREAD_PTHREADS:
        break;

    /* Not compiled with any multi-threading support. */
    case CCLOSURE_THREAD_NONE:
        break;
}
```

## Example

Suppose an external API provides some function that accepts a callback function:

```c
/* list.h */
#include <stdbool.h>
#include <stddef.h>

typedef void List;

List *ListCreate(size_t num, ...);
void ListForEach(List *list, bool (*callback)(int *element));
```

Functions like this typically accept a "data" parameter to pass to callback in addition to `element`, but imagine that isn't the case here. That functionality can be recreated using a closure:

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

/* Function that accepts closure context as first parameter. */
static bool SumGreaterThanCallback(CClosureCtx ctx, int *element) {
    /* Closure context contains the bound environment. */
    struct SumGreaterThanEnv *env = ctx.env;

    if (*element > env->threshold)
        env->sum += *element;

    return true;
}

int main(void) {
    List *list = ListCreate(5, 3, -10, 77, 42, 15);

    /* Instantiate an environment for the closure. */
    struct SumGreaterThanEnv *env = malloc(sizeof(struct SumGreaterThanEnv));
    *env = (SumGreaterThanEnv){
        .sum = 0,
        .threshold = 10
    };

    /* Create a closure by binding environment to callback. */
    bool (*callback)(int *) = CClosureNew(SumGreaterThanCallback, env, false);

    /* Callback is now a first-class function that will be passed environment
       implicitly as its first parameter. */
    ListForEach(list, callback);

    /* Would print "134". */
    printf("%i", env->sum);

    /* "CClosureFree" returns the closure's environment. */
    free(CClosureFree(callback));

    return 0;
}
```