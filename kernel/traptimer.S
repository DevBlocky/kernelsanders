.extern panic
.extern _spin

// trap handler for hart timer
//
// timers can only received in machine mode, hence
// the need for a separate trap handler
.section .text._traptimer
.globl _traptimer
.align 4
_traptimer:
    // push program state
    csrrw a0, mscratch, a0 // swap a0 and mscratch
    sd a1, 0(a0)
    sd a2, 8(a0)
    sd a3, 16(a0)

    // verify this is a timer interrupt
    // if this is a timer interrupt, mcause == 0x7 | (1 << 63)
    li a1, 0x8000000000000007
    csrr a2, mcause
    beq a1, a2, .L_timerintr

    // if we reach here, registers don't matter since we're
    // panicking
    la a0, .L_panicmsg
    call panic
    j _spin

.L_timerintr:
    // setup next timer
    ld a1, 24(a0) // CLINT_MTIMECMP addr
    ld a2, 32(a0) // timer interval
    ld a3, 0(a1) // *CLINT_MTIMECMP
    add a3, a3, a2
    sd a3, 0(a1)

    // setup software interrupt for supervisor mode
    csrr a1, sip
    ori a1, a1, 2
    csrw sip, a1

    // pop program state
    ld a1, 0(a0)
    ld a2, 8(a0)
    ld a3, 16(a0)
    csrrw a0, mscratch, a0 // swap back a0 and mscratch
    mret

.L_panicmsg:
    .string "_traptimer with non-timer cause"
