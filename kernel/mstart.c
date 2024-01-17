#include "entry.h"
#include "types.h"
#include "riscv.h"

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

    // jump to main
    asm("mret");
}
