#ifndef __RISCV_H
#define __RISCV_H

#include "types.h"

static inline void w_mtvec(uint64_t vector)
{
    asm("csrw mtvec, %0" : : "r"(vector));
}

#define MSTATUS_MPP_MASK (3 << 11)
#define MSTATUS_MPP_S (1 << 11)
static inline usize_t r_mstatus()
{
    usize_t mstatus;
    asm("csrr %0, mstatus" : "=r"(mstatus));
    return mstatus;
}
static inline void w_mstatus(usize_t mstatus)
{
    asm("csrw mstatus, %0" : : "r"(mstatus));
}

static inline void w_mepc(uint64_t addr)
{
    asm("csrw mepc, %0" : : "r"(addr));
}

static inline void w_mideleg(usize_t flags)
{
    asm("csrw mideleg, %0" : : "r"(flags));
}
static inline void w_medeleg(usize_t flags)
{
    asm("csrw medeleg, %0" : : "r"(flags));
}

static inline void w_pmpaddr0(usize_t addr)
{
    asm("csrw pmpaddr0, %0" : : "r"(addr));
}
static inline void w_pmpcfg0(usize_t cfg)
{
    asm("csrw pmpcfg0, %0" : : "r"(cfg));
}

#endif // __RISCV_H
