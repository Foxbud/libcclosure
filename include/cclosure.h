/**
 * @file
 *
 * @brief Utilities for interacting with function closures.
 *
 * @since 1.0.0
 *
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

/* ----- PUBLIC MACROS ----- */

#define CClosurePacked __attribute__((packed))

/* ----- PUBLIC TYPES ------ */

/**
 * @brief Closure context passsed as the first argument to every closure.
 *
 * @since 1.0.0
 */
typedef struct CClosurePacked CClosureCtx {
    // #undef CClosureCtx
    /**
     * @brief The environment bound to the current closure.
     *
     * @since 1.0.0
     */
    void *env;
#ifdef __LP64__
    /**
     * @internal
     */
    void *pad[2];
#endif
    /**
     * @internal
     */
    const void *const ret;
} CClosureCtx;

/**
 * @brief Possible values of ::CCLOSURE_THREAD_TYPE.
 *
 * @since 1.0.0
 *
 * @sa ::CCLOSURE_THREAD_TYPE
 */
typedef enum CClosureThreadType {
    /**
     * @brief libcclosure was **not** compiled with multi-threading support.
     *
     * @since 1.0.0
     */
    CCLOSURE_THREAD_NONE = 0,
    /**
     * @brief libcclosure was compiled with multi-threading support using POSIX
     * threads.
     *
     * @since 1.0.0
     */
    CCLOSURE_THREAD_PTHREADS,
} CClosureThreadType;

/* ----- PUBLIC CONSTANTS ----- */

/**
 * @brief Which type of multi-threading support libcclosure was compiled with.
 *
 * @since 1.0.0
 *
 * @sa CClosureThreadType
 */
extern const CClosureThreadType CCLOSURE_THREAD_TYPE;

/* ----- PUBLIC FUNCTIONS ----- */

/**
 * @brief Create a new closure by binding an environment to a function.
 *
 * @remark This function is completely thread-safe.
 * @remark The closure returned by this function is thread-safe if both
 * argument `env` is treated as readonly or is mediated by a lock, and the
 * situation mentioned in @ref CClosureFreeWarn does not apply.
 *
 * @param[in] fcn Pointer to the function to bind to. Its first parameter *must*
 * be of type CClosureCtx.
 * @param[in] env Environment to bind to. May be `NULL`.
 * @param[in] aggRet Wether the return type of argument `fcn` is an aggregate
 * (`true`) or a scalar (`false`).
 *
 * @return Pointer to newly bound closure. It will have the same signature as
 * argument `fcn` but without the first context parameter. This closure should
 * later be destroyed using ::CClosureFree.
 *
 * @since 1.0.0
 *
 * @sa CClosureFree
 */
void *CClosureNew(void *fcn, void *env, bool aggRet);

/**
 * @brief Destroy a closure previously created using ::CClosureNew.
 *
 * @subsubsection CClosureFreeWarn Thread-Safety Warning
 *
 * While this function is thread-safe in the majority of cases, **it is
 * not thread-safe and will result in undefined behavior** if it is called
 * while argument `clos`:
 * - is being or gets called,
 * - is being called with or gets passed to ::CClosureGetFcn,
 * - or is being called with or gets passed to ::CClosureGetEnv.
 *
 * @param[in] clos Closure to destroy.
 *
 * @return The environment previously bound to argument `clos`.
 *
 * @since 1.0.0
 *
 * @sa CClosureNew
 */
void *CClosureFree(void *clos);

/**
 * @brief Query whether or not a given reference points to an initialized
 * closure created using ::CClosureNew.
 *
 * @remark This function is completely thread-safe.
 *
 * @param[in] clos Reference to query.
 *
 * @return Whether or not argument `clos` is a valid closure.
 *
 * @since 1.0.0
 */
bool CClosureCheck(void *clos);

/**
 * @brief Query the callback function bound to a closure.
 *
 * @remark This function is thread-safe if the situation mentioned in
 * @ref CClosureFreeWarn does not apply.
 *
 * @param[in] clos Closure to query.
 *
 * @return Closure's bound callback function.
 *
 * @since 1.0.0
 */
void *CClosureGetFcn(void *clos);

/**
 * @brief Query the environment bound to a closure.
 *
 * @remark This function is thread-safe if the situation mentioned in
 * @ref CClosureFreeWarn does not apply.
 *
 * @param[in] clos Closure to query.
 *
 * @return Closure's bound environment.
 *
 * @since 1.0.0
 */
void *CClosureGetEnv(void *clos);

#endif /* CCLOSURE_H */