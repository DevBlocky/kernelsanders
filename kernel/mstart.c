#include "entry.h"
#include "types.h"
#include "riscv.h"

void timerinit(void);
void main(void);

__attribute__((aligned(16))) char stack0[STACK0_SIZE];

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


void _traptimer();
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
    w_mie(MIE_MTIE);
    w_mstatus(r_mstatus() | MSTATUS_MPIE);
}
