/*
 * Copyright 2022 Garrett Fairburn <breadboardfox@gmail.com>
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
#define IsAggRet(clos) (false)

#define THUNK_ENTRY_SIZE 26
#define THUNK_EXIT_SIZE 8
#else
#define IsAggRet(clos) (clos->entry.bin[0] == 0x5a)

#define THUNK_ENTRY_SIZE 14
#define THUNK_EXIT_SIZE 6
#endif

/* ----- PRIVATE TYPES ----- */

typedef struct __attribute__((packed)) Closure {
#ifdef __LP64__
    union {
        uint8_t bin[THUNK_ENTRY_SIZE];
        union {
            struct __attribute__((packed)) {
                uint8_t pad0[6];
                void* env;
                uint8_t pad1[4];
                void* fcn;
            } norm, agg;
        } tmpl;
    } entry;
    const uint8_t exit[THUNK_EXIT_SIZE];
#else
    union {
        uint8_t bin[THUNK_ENTRY_SIZE];
        union {
            struct __attribute__((packed)) {
                uint8_t pad0[1];
                void* env;
                uint8_t pad1[1];
                void* fcn;
                uint8_t pad2[4];
            } norm;
            struct __attribute__((packed)) {
                uint8_t pad0[4];
                void* env;
                uint8_t pad1[2];
                void* fcn;
            } agg;
        } tmpl;
    } entry;
    const uint8_t exit[THUNK_EXIT_SIZE];
#endif
} Closure;

typedef struct MemSlot {
    Closure clos;
    struct MemSlot* nextFree;
    const size_t blockIdx;
} MemSlot;

typedef struct MemBlock {
    const size_t rawSize;
    MemSlot* firstFree;
    MemSlot* const slots;
#ifdef THREAD_PTHREADS
    pthread_rwlock_t lock;
#endif
} MemBlock;

typedef struct MemBank {
    size_t cap;
    size_t size;
    MemBlock* blocks;
#ifdef THREAD_PTHREADS
    pthread_rwlock_t lock;
#endif
} MemBank;

/* ----- PRIVATE CONSTANTS ----- */

#ifdef __LP64__
/* BITS 64
 *
 * %define tmpl_env strict QWORD 0
 * %define tmpl_fcn strict QWORD 0
 *
 * thunk_entry_norm_x86_64:
 * 		sub rsp, 8 * 2
 * 		mov r11, tmpl_env
 * 		push r11
 * 		mov r11, tmpl_fcn
 */
static const uint8_t THUNK_ENTRY_NORM[THUNK_ENTRY_SIZE] = {
    0x48, 0x83, 0xec, 0x10, 0x49, 0xbb, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x53, 0x49, 0xbb,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t* THUNK_ENTRY_AGG = THUNK_ENTRY_NORM;

/* BITS 64
 *
 * thunk_entry_uninit_x86_64:
 * 		times 24 nop
 * 		ud2
 */
static const uint8_t THUNK_ENTRY_UNINIT[THUNK_ENTRY_SIZE] = {
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x0f, 0x0b};

/* BITS 64
 *
 * thunk_exit_x86_64:
 * 		call r11
 * 		add rsp, 8 * 3
 * 		ret
 */
static const uint8_t THUNK_EXIT[THUNK_EXIT_SIZE] = {0x41, 0xff, 0xd3, 0x48,
                                                    0x83, 0xc4, 0x18, 0xc3};
#else
/* BITS 32
 *
 * %define tmpl_env strict DWORD 0
 * %define tmpl_fcn strict DWORD 0
 *
 * thunk_entry_norm_x86:
 *      push tmpl_env
 * 		mov ecx, tmpl_fcn
 * 		jmp thunk_entry_uninit_x86
 * 		ud2;
 * thunk_entry_uninit_x86:
 */
static const uint8_t THUNK_ENTRY_NORM[THUNK_ENTRY_SIZE] = {
    0x68, 0x00, 0x00, 0x00, 0x00, 0xb9, 0x00,
    0x00, 0x00, 0x00, 0xeb, 0x02, 0x0f, 0x0b};

/* BITS 32
 *
 * %define tmpl_env strict DWORD 0
 * %define tmpl_fcn strict DWORD 0
 *
 * thunk_entry_agg_x86:
 * 		pop edx
 * 		pop ecx
 * 		push edx
 * 		push tmpl_env
 * 		push ecx
 * 		mov ecx, tmpl_fcn
 */
static const uint8_t THUNK_ENTRY_AGG[THUNK_ENTRY_SIZE] = {
    0x5a, 0x59, 0x52, 0x68, 0x00, 0x00, 0x00,
    0x00, 0x51, 0xb9, 0x00, 0x00, 0x00, 0x00};

/* BITS 32
 *
 * thunk_entry_uninit_x86:
 * 		times 12 nop
 * 		ud2
 */
static const uint8_t THUNK_ENTRY_UNINIT[THUNK_ENTRY_SIZE] = {
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x0f, 0x0b};

/* BITS 32
 *
 * thunk_exit_x86:
 *  		call ecx
 *  		add esp, 4
 *  		ret
 */
static const uint8_t THUNK_EXIT[THUNK_EXIT_SIZE] = {0xff, 0xd1, 0x83,
                                                    0xc4, 0x04, 0xc3};
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
static void UnlockRwLock(void* lock) {
    pthread_rwlock_unlock((pthread_rwlock_t*)lock);

    return;
}
#endif

static void MemBlockInit(MemBlock* block, size_t blockIdx) {
#ifdef THREAD_PTHREADS
    pthread_rwlock_init(&block->lock, NULL);
#endif
    *(size_t*)&block->rawSize = getpagesize()
                                << ((blockIdx > 11) ? 11 : blockIdx);
    size_t cap = block->rawSize / sizeof(MemSlot);
    *(MemSlot**)&block->slots =
        mmap(NULL, block->rawSize, PROT_READ | PROT_WRITE | PROT_EXEC,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    block->firstFree = block->slots + 0;

    for (size_t idx = 0; idx < cap - 1; idx++) {
        MemSlot* slot = block->slots + idx;
        memcpy(slot->clos.entry.bin, THUNK_ENTRY_UNINIT, THUNK_ENTRY_SIZE);
        memcpy((void*)slot->clos.exit, THUNK_EXIT, THUNK_EXIT_SIZE);
        slot->nextFree = slot + 1;
        *(size_t*)&slot->blockIdx = blockIdx;
    }
    MemSlot* last = block->slots + cap - 1;
    memcpy(last->clos.entry.bin, THUNK_ENTRY_UNINIT, THUNK_ENTRY_SIZE);
    memcpy((void*)last->clos.exit, THUNK_EXIT, THUNK_EXIT_SIZE);
    last->nextFree = NULL;
    *(size_t*)&last->blockIdx = blockIdx;

    return;
}

static void MemBlockDeinit(MemBlock* block) {
    munmap(block->slots, block->rawSize);
#ifdef THREAD_PTHREADS
    pthread_rwlock_destroy(&block->lock);
#endif

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

CCLOSURE_EXPORT void* CClosureNew(void* fcn, void* env, bool aggRet) {
    /* Find block with free slot. */
    MemBlock* block = NULL;
#ifdef THREAD_PTHREADS
    int32_t origCancelState;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &origCancelState);
    pthread_rwlock_rdlock(&bank.lock);
#endif
    for (size_t idx = 0; idx < bank.size; idx++) {
        MemBlock* curBlock = bank.blocks + idx;
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
    MemSlot* slot = block->firstFree;
    block->firstFree = slot->nextFree;
    slot->nextFree = NULL;
#ifdef THREAD_PTHREADS
    pthread_rwlock_unlock(&block->lock);
    pthread_rwlock_unlock(&bank.lock);
    pthread_setcancelstate(origCancelState, &origCancelState);
#endif

    /* Initialize closure entry. */
    Closure* clos = (Closure*)slot;
    if (aggRet) {
        memcpy(clos->entry.bin, THUNK_ENTRY_AGG, THUNK_ENTRY_SIZE);
        clos->entry.tmpl.agg.fcn = fcn;
        clos->entry.tmpl.agg.env = env;
    } else {
        memcpy(clos->entry.bin, THUNK_ENTRY_NORM, THUNK_ENTRY_SIZE);
        clos->entry.tmpl.norm.fcn = fcn;
        clos->entry.tmpl.norm.env = env;
    }

    return clos;
}

CCLOSURE_EXPORT void* CClosureFree(void* clos) {
#define clos ((Closure*)clos)
    /* Deinitialize closure entry. */
    void* env =
        (IsAggRet(clos)) ? clos->entry.tmpl.agg.env : clos->entry.tmpl.norm.env;
#ifdef THREAD_PTHREADS
    int32_t origCancelState;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &origCancelState);
#endif
    memcpy(clos->entry.bin, THUNK_ENTRY_UNINIT, THUNK_ENTRY_SIZE);

    /* Release free slot. */
    MemSlot* slot = (MemSlot*)clos;
#ifdef THREAD_PTHREADS
    pthread_rwlock_rdlock(&bank.lock);
#endif
    MemBlock* block = bank.blocks + slot->blockIdx;
#ifdef THREAD_PTHREADS
    pthread_rwlock_wrlock(&block->lock);
#endif
    slot->nextFree = block->firstFree;
    block->firstFree = slot;
#ifdef THREAD_PTHREADS
    pthread_rwlock_unlock(&block->lock);
    pthread_rwlock_unlock(&bank.lock);
    pthread_setcancelstate(origCancelState, &origCancelState);
#endif

    return env;
#undef clos
}

CCLOSURE_EXPORT bool CClosureCheck(void* clos) {
    bool result = false;
#ifdef THREAD_PTHREADS
    pthread_rwlock_rdlock(&bank.lock);
    pthread_cleanup_push(UnlockRwLock, &bank.lock);
#endif
    for (size_t idx = 0; idx < bank.size; idx++) {
        MemBlock* block = bank.blocks + idx;
        void* slots = block->slots;
        if ((clos >= slots) && (clos < slots + block->rawSize)) {
#ifdef THREAD_PTHREADS
            pthread_rwlock_rdlock(&block->lock);
            pthread_cleanup_push(UnlockRwLock, &block->lock);
#endif
            result = ((Closure*)clos)->entry.bin[0] != 0x90;
#ifdef THREAD_PTHREADS
            pthread_cleanup_pop(true);
#endif
        }
        if (result)
            break;
    }
#ifdef THREAD_PTHREADS
    pthread_cleanup_pop(true);
#endif

    return result;
}

CCLOSURE_EXPORT void* CClosureGetFcn(void* clos) {
#define clos ((Closure*)clos)
    return (IsAggRet(clos)) ? clos->entry.tmpl.agg.fcn
                            : clos->entry.tmpl.norm.fcn;
#undef clos
}

CCLOSURE_EXPORT void* CClosureGetEnv(void* clos) {
#define clos ((Closure*)clos)
    return (IsAggRet(clos)) ? clos->entry.tmpl.agg.env
                            : clos->entry.tmpl.norm.env;
#undef clos
}