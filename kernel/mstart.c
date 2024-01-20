#include "entry.h"
#include "types.h"
#include "riscv.h"

void timerinit(void);
void main(void);

__attribute__((aligned(16))) char stack0[STACK0_SIZE];

#define CLINT 0x2000000
// for some reason, the locations of these aren't documented well
// I only found this at section 6.1:
// https://sifive.cdn.prismic.io/sifive%2Fc89f6e5a-cf9e-44c3-a3db-04420702dcc1_sifive+e31+manual+v19.08.pdf
#define CLINT_MTIMECMP (volatile uint64_t*)(CLINT + 0x4000)
#define CLINT_MTIME (volatile uint64_t*)(CLINT + 0xBFF8)

void mstart(char *dtb)
{
    // GOAL: get out of machine mode asap

    // set next privilege to S mode
    usize_t status = r_mstatus();
    status &= ~MSTATUS_MPP_MASK;
    status |= MSTATUS_MPP_S;
    w_mstatus(status);

    // set mepc (mret will jump here)
    w_mepc((uint64_t)main);

    // delegate all interrupts and exceptions to S mode
    w_medeleg(0xffff);
    w_mideleg(0xffff);

    // grant full memory access to S mode
    w_pmpaddr0(~0);
    w_pmpcfg0(0xf);

    // init timer interrupts
    timerinit();

    // jump to main
    asm("mret");
}


extern void _traptimer();
uint64_t timerscratch[5];

// since the CLINT can only produce machine-mode interrupts,
// we must initialize the timer and take the interrupt in
// machine mode
//
// once a timer interrupt is taken, we pass a software interrupt
// to supervisor mode
void timerinit(void) {
    uint64_t interval = 1000000;
    *CLINT_MTIMECMP = *CLINT_MTIME + interval;

    // setup scratch
    // mscratch[0..=2] = register scratch
    // mscratch[3] = addr of CLINT_MTIMECMP
    // mscratch[4] = our desired interval
    timerscratch[3] = (uint64_t)CLINT_MTIMECMP;
    timerscratch[4] = interval;
    w_mscratch((uint64_t)&timerscratch[0]);

    // setup interrupt handler
    w_mtvec((uint64_t)_traptimer);
    w_mstatus(r_mstatus() | MSTATUS_MPIE);
    w_mie(MIE_MTIE);
}
