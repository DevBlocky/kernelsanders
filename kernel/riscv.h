#ifndef __RISCV_H
#define __RISCV_H

#include "types.h"

#define CLINT 0x2000000
// for some reason, the offsets of CLINT registers are not well documented
// (especially the CLINT_MTIME) I could only find it here (section 6.1):
// https://sifive.cdn.prismic.io/sifive%2Fc89f6e5a-cf9e-44c3-a3db-04420702dcc1_sifive+e31+manual+v19.08.pdf
#define CLINT_MTIMECMP (volatile uint64_t *)(CLINT + 0x4000)
#define CLINT_MTIME (volatile uint64_t *)(CLINT + 0xBFF8)

#define UART0 0x10000000

//
// machine-mode csr registers
//

#define MSTATUS_MIE (1 << 3)
#define MSTATUS_MPIE (1 << 7)
#define MSTATUS_MPP_MASK (3 << 11)
#define MSTATUS_MPP_S (1 << 11)

#define MIE_MTIE (1 << 7)

static inline usize_t r_mstatus()
{
    usize_t status;
    asm("csrr %0, mstatus" : "=r"(status));
    return status;
}
static inline void w_mstatus(usize_t mstatus)
{
    asm("csrw mstatus, %0" : : "r"(mstatus));
}

static inline void w_mtvec(usize_t vector)
{
    asm("csrw mtvec, %0" : : "r"(vector));
}
static inline void w_mepc(usize_t addr)
{
    asm("csrw mepc, %0" : : "r"(addr));
}
static inline void w_mie(usize_t flags)
{
    asm("csrw mie, %0" : : "r"(flags));
}
static inline void w_mscratch(usize_t any)
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

//
// supervisor-mode csr registers
//

#define SSTATUS_SIE (1 << 1)

#define SIP_SSIP (1 << 1)
#define SIE_SSIE (1 << 1)

static inline usize_t r_sstatus()
{
    usize_t status;
    asm("csrr %0, sstatus" : "=r"(status));
    return status;
}
static inline void w_sstatus(usize_t status)
{
    asm("csrw sstatus, %0" : : "r"(status));
}

static inline void w_stvec(usize_t addr)
{
    asm("csrw stvec, %0" : : "r"(addr));
}
static inline void w_sie(usize_t flags)
{
    asm("csrw sie, %0" : : "r"(flags));
}
static inline usize_t r_sip()
{
    usize_t flags;
    asm("csrr %0, sip" : "=r"(flags));
    return flags;
}
static inline void w_sip(usize_t flags)
{
    asm("csrw sip, %0" : : "r"(flags));
}

static inline usize_t r_scause()
{
    usize_t code;
    asm("csrr %0, scause" : "=r"(code));
    return code;
}

static inline void intr_on()
{
    w_sstatus(r_sstatus() | SSTATUS_SIE);
}
static inline void intr_off()
{
    w_sstatus(r_sstatus() & ~SSTATUS_SIE);
}

#endif // __RISCV_H
