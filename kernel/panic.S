.extern printf

.section .text._panic
.globl _panic
_panic:
    // disable virtual memory / paging
    sfence.vma zero, zero
    csrw satp, zero
    sfence.vma zero, zero

    // atomically swap panicking
    li t0, 1
    la t1, panicking
    amoswap.w t0, t0, (t1)
    // spin core if already panicked
    bnez t0, _spin

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
