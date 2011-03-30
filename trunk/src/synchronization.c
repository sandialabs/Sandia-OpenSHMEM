/* -*- C -*-
 *
 * Copyright (c) 2011 Sandia National Laboratories. All rights reserved.
 */

#include "config.h"

#include <portals4.h>
#include <portals4_runtime.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include "mpp/shmem.h"
#include "shmem_internal.h"

static long *barrier_work_array;

int
shmem_barrier_init(void)
{
    barrier_work_array = shmalloc(sizeof(long) * _SHMEM_BARRIER_SYNC_SIZE);
    if (NULL == barrier_work_array) return -1;
    bzero(barrier_work_array, sizeof(long) * _SHMEM_BARRIER_SYNC_SIZE);
    return 0;
}


void
shmem_barrier(int PE_start, int logPE_stride, int PE_size, long *pSync)
{
    int stride = (logPE_stride == 0) ? 1 : 1 << logPE_stride;
    if (PE_start == shmem_int_my_pe) {
        int pe, i;
        shmem_long_wait_until(pSync, SHMEM_CMP_EQ, PE_size - 1);
        pSync[0] = 0;
        for (pe = PE_start + stride, i = 1 ; 
             i < PE_size ;  
             i++, pe += stride) {
            shmem_long_p(pSync, 1, pe);
        }
    } else {
        shmem_long_inc(pSync, PE_start);
        shmem_long_wait(pSync, 0);
        pSync[0] = 0;
    }
}


void
shmem_barrier_all(void)
{
    shmem_quiet();
    shmem_barrier(0, 0, shmem_int_num_pes, barrier_work_array);
}


void
shmem_quiet(void)
{
    int ret;
    ptl_ct_event_t ct;

    /* wait for remote completion (acks) of all pending events */
    ret = PtlCTWait(put_ct_h, pending_put_counter, &ct);
    if (PTL_OK != ret) { abort(); }
    if (ct.failure != 0) { abort(); }
}


void
shmem_fence(void)
{
    /* intentionally a no-op */
}


#define COMP(type, a, b, ret)                            \
    do {                                                 \
        ret = 0;                                         \
        switch (type) {                                  \
        case SHMEM_CMP_EQ:                               \
            if (a == b) ret = 1;                         \
            break;                                       \
        case SHMEM_CMP_NE:                               \
            if (a != b) ret = 1;                         \
            break;                                       \
        case SHMEM_CMP_GT:                               \
            if (a > b) ret = 1;                          \
            break;                                       \
        case SHMEM_CMP_GE:                               \
            if (a >= b) ret = 1;                         \
            break;                                       \
        case SHMEM_CMP_LT:                               \
            if (a < b) ret = 1;                          \
            break;                                       \
        case SHMEM_CMP_LE:                               \
            if (a <= b) ret = 1;                         \
            break;                                       \
        default:                                         \
            abort();                                     \
        }                                                \
    } while(0)


#define SHMEM_WAIT(var, value)                           \
    do {                                                 \
        int ret;                                         \
        ptl_ct_event_t ct;                               \
                                                         \
        while (*var == value) {                          \
            ret = PtlCTGet(target_ct_h, &ct);            \
            if (PTL_OK != ret) { abort(); }              \
            if (*var != value) return;                   \
            ret = PtlCTWait(target_ct_h,                 \
                            ct.success + ct.failure + 1, \
                            &ct);                        \
            if (PTL_OK != ret) { abort(); }              \
        }                                                \
    } while(0)


#define SHMEM_WAIT_UNTIL(var, cond, value)               \
    do {                                                 \
        int ret;                                         \
        ptl_ct_event_t ct;                               \
        int cmpval;                                      \
                                                         \
        COMP(cond, *var, value, cmpval);                 \
        while (!cmpval) {                                \
            ret = PtlCTGet(target_ct_h, &ct);            \
            if (PTL_OK != ret) { abort(); }              \
            COMP(cond, *var, value, cmpval);             \
            if (cmpval) return;                          \
            ret = PtlCTWait(target_ct_h,                 \
                            ct.success + ct.failure + 1, \
                            &ct);                        \
            if (PTL_OK != ret) { abort(); }              \
            COMP(cond, *var, value, cmpval);             \
        }                                                \
    } while(0)


void
shmem_short_wait(short *var, short value)
{
    SHMEM_WAIT(var, value);
}


void
shmem_short_wait_until(short *var, int cond, short value)
{
    SHMEM_WAIT_UNTIL(var, cond, value);
}


void
shmem_int_wait(int *var, int value)
{
    SHMEM_WAIT(var, value);
}


void
shmem_int_wait_until(int *var, int cond, int value)
{
    SHMEM_WAIT_UNTIL(var, cond, value);
}


void
shmem_long_wait(long *var, long value)
{
    SHMEM_WAIT(var, value);
}


void
shmem_long_wait_until(long *var, int cond, long value)
{
    SHMEM_WAIT_UNTIL(var, cond, value);
}


void
shmem_longlong_wait(long long *var, long long value)
{
    SHMEM_WAIT(var, value);
}


void
shmem_longlong_wait_until(long long *var, int cond, long long value)
{
    SHMEM_WAIT_UNTIL(var, cond, value);
}


void
shmem_wait(long *ivar, long cmp_value)
{
    SHMEM_WAIT(ivar, cmp_value);
}


void
shmem_wait_until(long *ivar, int cmp, long value)
{
    SHMEM_WAIT_UNTIL(ivar, cmp, value);
}
