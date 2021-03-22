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
#define _GNU_SOURCE 1

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef THREAD_PTHREADS
#include <pthread.h>
#endif

#undef _GNU_SOURCE

#include "cclosure.h"
#include "export.h"

/* ----- PRIVATE MACROS ----- */

#ifdef __LP64__
#define THUNK_SIZE 0x15
#else
#define THUNK_SIZE 0x15
#endif

/* ----- PRIVATE TYPES ----- */

typedef struct __attribute__((packed)) Closure {
    uint8_t thunk[THUNK_SIZE];
    void *fcn;
    void *env;
} Closure;

typedef struct MemSlot {
    Closure clos;
    struct MemSlot *nextFree;
    size_t blockIdx;
} MemSlot;

typedef struct MemBlock {
    size_t rawSize;
    MemSlot *firstFree;
    MemSlot *slots;
#ifdef THREAD_PTHREADS
    pthread_rwlock_t lock;
#endif
} MemBlock;

typedef struct MemBank {
    size_t cap;
    size_t size;
    MemBlock *blocks;
#ifdef THREAD_PTHREADS
    pthread_rwlock_t lock;
#endif
} MemBank;

/* ----- PRIVATE CONSTANTS ----- */

#ifdef __LP64__
/**
 * sub rsp, 8 * 2
 * push [rip+env]
 * call [rip+fcn]
 * add rsp, 8 * 3
 * ret
 * fcn:
 * nop;nop;nop;nop;nop;nop;nop;nop;
 * env:
 * nop;nop;nop;nop;nop;nop;nop;nop;
 */
static const uint8_t THUNK_NORM_RET[THUNK_SIZE] = {
    0x48, 0x83, 0xEC, 0x10, 0xFF, 0x35, 0x13, 0x00, 0x00, 0x00, 0xFF,
    0x15, 0x05, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x18, 0xC3};

static const uint8_t *THUNK_AGG_RET = THUNK_NORM_RET;
#else
/**
 * call get_pc_thunk
 * rel:
 * push [eax+(env-rel)]
 * call [eax+(fcn-rel)]
 * pop ecx
 * ret
 * get_pc_thunk:
 * mov eax, [esp]
 * ret
 * nop;nop;nop;nop
 * fcn:
 * nop;nop;nop;nop
 * env:
 * nop;nop;nop;nop
 */
static const uint8_t THUNK_NORM_RET[THUNK_SIZE] = {
    0xE8, 0x08, 0x00, 0x00, 0x00, 0xFF, 0x70, 0x14, 0xFF, 0x50, 0x10,
    0x59, 0xC3, 0x8B, 0x04, 0x24, 0xC3, 0x90, 0x90, 0x90, 0x90};

/**
 * call get_pc_thunk
 * rel:
 * pop edx
 * pop ecx
 * push edx
 * push [eax+(env-rel)]
 * push ecx
 * call [eax+(fcn-rel)]
 * pop ecx
 * ret
 * get_pc_thunk:
 * mov eax, [esp]
 * ret
 * fcn:
 * nop;nop;nop;nop
 * env:
 * nop;nop;nop;nop
 */
static const uint8_t THUNK_AGG_RET[THUNK_SIZE] = {
    0xE8, 0x0C, 0x00, 0x00, 0x00, 0x5A, 0x59, 0x52, 0xFF, 0x70, 0x14,
    0x51, 0xFF, 0x50, 0x10, 0x59, 0xC3, 0x8B, 0x04, 0x24, 0xC3};
#endif

/* ----- PUBLIC CONSTANTS ----- */

CCLOSURE_EXPORT const CClosureThreadType CCLOSURE_THREAD_TYPE =
#ifdef THREAD_PTHREADS
    CCLOSURE_THREAD_PTHREADS;
#else
    CCLOSURE_THREAD_NONE;
#endif

/* ----- PRIVATE GLOBALS ----- */

static MemBank bank = {0};

/* ----- PRIVATE FUNCTIONS ----- */

#ifdef THREAD_PTHREADS
static void UnlockRwLock(void *lock) {
    pthread_rwlock_unlock((pthread_rwlock_t *)lock);

    return;
}
#endif

static void MemBlockInit(MemBlock *block, size_t blockIdx) {
#ifdef THREAD_PTHREADS
    pthread_rwlock_init(&block->lock, NULL);
#endif
    block->rawSize = getpagesize() << ((blockIdx > 11) ? 11 : blockIdx);
    size_t cap = block->rawSize / sizeof(MemSlot);
    block->slots =
        mmap(NULL, block->rawSize, PROT_READ | PROT_WRITE | PROT_EXEC,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    block->firstFree = block->slots + 0;

    for (size_t idx = 0; idx < cap - 1; idx++) {
        MemSlot *slot = block->slots + idx;
        slot->nextFree = slot + 1;
        slot->blockIdx = blockIdx;
    }
    MemSlot *last = block->slots + cap - 1;
    last->nextFree = NULL;
    last->blockIdx = blockIdx;

    return;
}

static void MemBlockDeinit(MemBlock *block) {
    munmap(block->slots, block->rawSize);
#ifdef THREAD_PTHREADS
    pthread_rwlock_destroy(&block->lock);
#endif
    *block = (MemBlock){0};

    return;
}

__attribute__((constructor)) static void Constructor(void) {
#ifdef THREAD_PTHREADS
    pthread_rwlock_init(&bank.lock, NULL);
#endif
    bank.cap = 32;
    bank.blocks = malloc(bank.cap * sizeof(MemBlock));
    bank.size = 1;
    MemBlockInit(bank.blocks + 0, 0);

    return;
}

__attribute__((destructor)) static void Destructor(void) {
    for (size_t idx = 0; idx < bank.size; idx++)
        MemBlockDeinit(bank.blocks + idx);
    free(bank.blocks);
#ifdef THREAD_PTHREADS
    pthread_rwlock_destroy(&bank.lock);
#endif
    bank = (MemBank){0};

    return;
}

/* ----- PUBLIC FUNCTIONS ----- */

CCLOSURE_EXPORT void *CClosureNew(void *fcn, void *env, bool aggRet) {
    /* Find block with free slot. */
    MemBlock *block = NULL;
#ifdef THREAD_PTHREADS
    int32_t origCancelState;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &origCancelState);
    pthread_rwlock_rdlock(&bank.lock);
#endif
    for (size_t idx = 0; idx < bank.size; idx++) {
        MemBlock *curBlock = bank.blocks + idx;
#ifdef THREAD_PTHREADS
        if (pthread_rwlock_trywrlock(&curBlock->lock) != 0)
            continue;
#endif
        if (curBlock->firstFree != NULL) {
            block = curBlock;
            break;
        }
#ifdef THREAD_PTHREADS
        pthread_rwlock_unlock(&curBlock->lock);
#endif
    }

    /* Create new block if none found. */
    if (block == NULL) {
#ifdef THREAD_PTHREADS
        pthread_rwlock_unlock(&bank.lock);
        pthread_rwlock_wrlock(&bank.lock);
#endif
        if (bank.size == bank.cap)
            bank.blocks =
                realloc(bank.blocks, (bank.cap *= 2) * sizeof(MemBlock));
        MemBlockInit((block = bank.blocks + bank.size), bank.size);
        bank.size++;
#ifdef THREAD_PTHREADS
        pthread_rwlock_wrlock(&block->lock);
#endif
    }

    /* Consume free slot. */
    MemSlot *slot = block->firstFree;
    block->firstFree = slot->nextFree;
    slot->nextFree = NULL;
#ifdef THREAD_PTHREADS
    pthread_rwlock_unlock(&block->lock);
    pthread_rwlock_unlock(&bank.lock);
    pthread_setcancelstate(origCancelState, &origCancelState);
#endif

    /* Initialize closure. */
    Closure *clos = (Closure *)slot;
    memcpy(clos->thunk, (aggRet) ? THUNK_AGG_RET : THUNK_NORM_RET,
           sizeof(clos->thunk));
    clos->fcn = fcn;
    clos->env = env;

    return clos;
}

CCLOSURE_EXPORT void *CClosureFree(void *clos) {
#define clos ((Closure *)clos)
    /* Deinitialize closure. */
    memset(clos->thunk, 0x0, sizeof(clos->thunk));
    clos->fcn = NULL;
    void *env = clos->env;
    clos->env = NULL;

    /* Release free slot. */
    MemSlot *slot = (MemSlot *)clos;
#ifdef THREAD_PTHREADS
    pthread_rwlock_rdlock(&bank.lock);
    pthread_cleanup_push(UnlockRwLock, &bank.lock);
#endif
    MemBlock *block = bank.blocks + slot->blockIdx;
#ifdef THREAD_PTHREADS
    int32_t origCancelState;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &origCancelState);
    pthread_rwlock_wrlock(&block->lock);
#endif
    slot->nextFree = block->firstFree;
    block->firstFree = slot;
#ifdef THREAD_PTHREADS
    pthread_rwlock_unlock(&block->lock);
    pthread_setcancelstate(origCancelState, &origCancelState);
    pthread_cleanup_pop(true);
#endif

    return env;
#undef clos
}

CCLOSURE_EXPORT bool CClosureCheck(void *clos) {
    bool result;
#ifdef THREAD_PTHREADS
    pthread_rwlock_rdlock(&bank.lock);
    pthread_cleanup_push(UnlockRwLock, &bank.lock);
#endif
    result = false;
    for (size_t idx = 0; idx < bank.size; idx++) {
        MemBlock *block = bank.blocks + idx;
        void *slots = block->slots;
        if ((result = (clos >= slots) && (clos < slots + block->rawSize) &&
                      (((Closure *)clos)->thunk[0] != 0)))
            break;
    }
#ifdef THREAD_PTHREADS
    pthread_cleanup_pop(true);
#endif

    return result;
}

CCLOSURE_EXPORT void *CClosureGetFcn(void *clos) {
    return ((Closure *)clos)->fcn;
}

CCLOSURE_EXPORT void *CClosureGetEnv(void *clos) {
    return ((Closure *)clos)->env;
}