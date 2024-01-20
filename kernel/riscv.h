#ifndef __RISCV_H
#define __RISCV_H

#include "types.h"

#define MSTATUS_MPIE (1 << 7)
#define MSTATUS_MPP_MASK (3 << 11)
#define MSTATUS_MPP_S (1 << 11)

#define MIE_MTIE (1 << 7)

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

static inline void w_mtvec(uint64_t vector)
{
    asm("csrw mtvec, %0" : : "r"(vector));
}
static inline void w_mepc(uint64_t addr)
{
    asm("csrw mepc, %0" : : "r"(addr));
}
static inline void w_mie(uint64_t flags)
{
    asm("csrw mie, %0" : : "r"(flags));
}
static inline void w_mscratch(uint64_t any)
{
    asm("csrw mscratch, %0" : : "r"(any));
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
