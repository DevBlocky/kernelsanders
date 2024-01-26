.extern _spin
.extern printf

.section .text.panic
.globl panic
panic:
    // disable virtual memory / paging
    mv t0, zero
    sfence.vma zero, zero
    csrw satp, t0
    sfence.vma zero, zero

    // print panic msg
    mv a1, a0
    la a0, .L_printfmt
    call printf

    // spin core
    j _spin
    j panic

.L_printfmt:
    .string "\n!!! kernel panic !!!\nmsg: %s\n"
