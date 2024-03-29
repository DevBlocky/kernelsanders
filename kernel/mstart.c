#include "entry.h"
#include "riscv.h"
#include "types.h"

static void timerinit(void);
void init(void *dtb);

__attribute__((aligned(16))) char stack0[STACK0_SIZE];

void mstart(void *dtb) {
  // GOAL: get out of machine mode asap

  // set next privilege to S mode
  usize status = r_mstatus();
  status &= ~MSTATUS_MPP_MASK;
  status |= MSTATUS_MPP_S;
  w_mstatus(status);

  // set mepc (mret will jump here)
  w_mepc((usize)init);

  // delegate all interrupts and exceptions to S mode
  w_medeleg(0xffff);
  w_mideleg(0xffff);

  // grant full memory access to S mode
  w_pmpaddr0(~0);
  w_pmpcfg0(0xf);

  // init timer interrupts
  timerinit();

  // jump to main (with dtb as arg)
  asm("mv a0, %0" : : "r"(dtb) : "a0");
  asm("mret");
}

void _traptimer();
u64 timerscratch[5];

// since the CLINT can only produce machine-mode interrupts,
// we must initialize the timer and take the interrupt in
// machine mode
//
// once a timer interrupt is taken, we pass a software interrupt
// to supervisor mode
static void timerinit(void) {
  u64 interval = 1000000;
  *CLINT_MTIMECMP = *CLINT_MTIME + interval;

  // setup scratch
  // mscratch[0..=2] = register scratch
  // mscratch[3] = addr of CLINT_MTIMECMP
  // mscratch[4] = our desired interval
  timerscratch[3] = (u64)CLINT_MTIMECMP;
  timerscratch[4] = interval;
  w_mscratch((usize)&timerscratch[0]);

  // setup interrupt handler
  w_mtvec((usize)_traptimer);
  w_mie(MIE_MTIE);
  w_mstatus(r_mstatus() | MSTATUS_MPIE);
}
