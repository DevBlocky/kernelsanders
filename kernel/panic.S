.extern printf

.section .text._panic
.globl _panic
_panic:
    // disable virtual memory / paging
    sfence.vma zero, zero
    csrw satp, zero
    sfence.vma zero, zero

    // spin core if already panicked
    lw t0, panicking
    bnez t0, _spin
    // set panicking to 1
    la t0, panicking
    li t1, 1
    sw t1, 0(t0)

    // print panic msg
    csrr a5, stval
    csrr a4, scause
    csrr a3, sepc
    mv a2, ra
    mv a1, a0
    la a0, .L_printfmt
    call printf

.globl _spin
_spin:
    wfi
    j _spin

.section .rodata
.L_printfmt:
    .string "\n!!! kernel panic !!!\nmsg: %s\nra: %hl\nsepc: %hl\nscause: %hl\nstval: %hl\n"

.section .data
.globl panicking
panicking:
    .word 0
