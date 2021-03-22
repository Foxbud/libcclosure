/**
 * @copyright 2021 Garrett Fairburn <breadboardfox@gmail.com>
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
#ifndef CCLOSURE_H
#define CCLOSURE_H

#ifndef __unix__
#error "cclosure.h only works on unix-like systems."
#endif /* unix */

#ifndef __i386__
#ifndef __x86_64__
#error "cclosure.h only works on i386 and x86_64 architectures."
#endif /* __x86_64__ */
#endif /* __i386__ */

#include <stdbool.h>
#include <stddef.h>

/* ----- PUBLIC TYPES ------ */

typedef struct __attribute__((packed)) CClosureCtx {
    void *env;
#ifdef __LP64__
    void *pad[2];
#endif
    const void *const ret;
} CClosureCtx;

typedef enum CClosureThreadType {
    CCLOSURE_THREAD_NONE = 0,
    CCLOSURE_THREAD_PTHREADS,
} CClosureThreadType;

/* ----- PUBLIC CONSTANTS ----- */

extern const CClosureThreadType CCLOSURE_THREAD_TYPE;

/* ----- PUBLIC FUNCTIONS ----- */

void *CClosureNew(void *fcn, void *env, bool aggRet);

void *CClosureFree(void *clos);

bool CClosureCheck(void *clos);

void *CClosureGetFcn(void *clos);

void *CClosureGetEnv(void *clos);

#endif /* CCLOSURE_H */