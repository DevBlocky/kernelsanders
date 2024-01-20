#include "entry.h"

.extern stack0
.extern mstart
.extern panic

.section .text._start
.globl _start
_start:
    // init stack (set to stack0)
    // we need to add 0x1000 (size of stack) because they grow upward
    la sp, stack0
    li a0, STACK0_SIZE
    add sp, sp, a0

    // add quick and dirty trap for debugging
    // if an exception/interrupt happens before being setup
    // in supervisor mode, then it'll be redirected to _trap
    la a0, _trapearly
    csrw mtvec, a0
    csrw stvec, a0

    // if mhartid != 0, spin core (we only want to use one core)
    csrr tp, mhartid
    bnez tp, _spin

    // a1 contains the address of the dtb, provided by qemu
    // mv to a0 so that it's a parameter of mstart
    mv a0, a1
    call mstart

.globl _spin
_spin:
    j _spin

.align 4
_trapearly:
    la a0, .L_panicmsg
    call panic
    j _spin
.L_panicmsg:
    .string "_trap hit unexpectedly"
